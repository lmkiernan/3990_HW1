#include <cstdint>
#include <cstdlib>

#include "HashTable.hpp"
#include "HashTable_priv.hpp"
#include "LinkedList.hpp"

///////////////////////////////////////////////////////////////////////////////
// Internal helper functions.
//
static constexpr size_t k_invalid_index = -1;

// Grows the hashtable (ie, increase the number of buckets) if its load
// factor has become too high.
static void MaybeResize(HashTable* ht);

// Implemented for you
static size_t HashKeyToBucketNum(HashTable* ht, HTHash_t hash) {
  return hash % ht->num_buckets;
}

// Deallocation functions that do nothing.  Useful if we want to deallocate
// the structure (eg, the linked list) without deallocating its elements or
// if we know that the structure is empty.
static void LLNoOpDelete(LLPayload_t delete_me) {}
static void HTNoOpDelete(HTKeyValue_t delete_me) {}

///////////////////////////////////////////////////////////////////////////////
// HashTable implementation.

// Implemented for you
HTHash_t FNVHash64(unsigned char* buffer, int len) {
  // This code is adapted from code by Landon Curt Noll
  // and Bonelli Nicola:
  //     http://code.google.com/p/nicola-bonelli-repo/
  static constexpr uint64_t FNV1_64_INIT = 0xcbf29ce484222325ULL;
  static constexpr uint64_t FNV_64_PRIME = 0x100000001b3ULL;
  unsigned char* bp = buffer;
  unsigned char* be = bp + len;
  uint64_t hval = FNV1_64_INIT;

  // FNV-1a hash each octet of the buffer.
  while (bp < be) {
    // XOR the bottom with the current octet.
    hval ^= static_cast<uint64_t>(*bp++);
    // Multiply by the 64 bit FNV magic prime mod 2^64.
    hval *= FNV_64_PRIME;
  }
  return hval;
}

// Implemented for you
HashTable* HashTable_New(size_t num_buckets, KeyCmpFnPtr key_compare_function) {
  // Allocate the hash table record.
  HashTable* ht = new HashTable{};

  // Initialize the record.
  ht->num_buckets = num_buckets;
  ht->num_elements = 0;
  ht->buckets = new LinkedList*[num_buckets];
  for (int i = 0; i < num_buckets; i++) {
    ht->buckets[i] = LinkedList_New();
  }

  ht->key_cmp_fn = key_compare_function;

  return ht;
}

// Implemented for you
void HashTable_Delete(HashTable* table, KeyValueFreeFnPtr kv_free_function) {
  int i;

  // Free each bucket's chain.
  for (i = 0; i < table->num_buckets; i++) {
    LinkedList* bucket = table->buckets[i];
    HTKeyValue_t* kv;

    // Pop elements off the chain list one at a time.  We can't do a single
    // call to LinkedList_Delete since we need to use the passed-in
    // value_free_function -- which takes a HTKeyValue_t, not an LLPayload_t --
    // to deallocate the caller's memory.
    while (LinkedList_NumElements(bucket) > 0) {
      LinkedList_Pop(bucket, reinterpret_cast<LLPayload_t*>(&kv));
      kv_free_function(*kv);
      delete kv;
    }
    // The chain is empty, so we can pass in the
    // null free function to LinkedList_Delete.
    LinkedList_Delete(bucket, LLNoOpDelete);
  }

  // Free the bucket array within the table, then deallocate the table record itself.
  delete[] table->buckets;
  delete table;
}

// Implemented for you
size_t HashTable_NumElements(HashTable* table) {
  return table->num_elements;
}

bool HashTable_Insert(HashTable* table,
                      HTKeyValue_t newkeyvalue,
                      HTKeyValue_t* oldkeyvalue) {
  MaybeResize(table);

  // Calculate which bucket and chain we're inserting into.
  const size_t bucket = HashKeyToBucketNum(table, newkeyvalue.hash);
  LinkedList* chain = table->buckets[bucket];

  // STEP 1: finish the implementation of InsertHashTable.
  // This is a fairly complex task, so you might decide you want
  // to define/implement a helper function that helps you find
  // and optionally remove a key within a chain, rather than putting
  // all that logic inside here.  You might also find that your helper
  // can be reused in steps 2 and 3.
  HTKeyValue_t* kv = new HTKeyValue_t();
  kv->hash = newkeyvalue.hash;
  kv->key = newkeyvalue.key;
  kv->value = newkeyvalue.value;
  LinkedListNode* node = chain->head;
  while (node != nullptr) {
    if (table->key_cmp_fn(node->payload->key, newkeyvalue.key)) {
      *oldkeyvalue = *reinterpret_cast<HTKeyValue_t*>(node->payload);
      node->payload = reinterpret_cast<LLPayload_t>(kv);
      return true;
    }
    node = node->next;
  }
  LinkedList_Push(chain, kv);
  table->num_elements++;
  return false;
}

bool HashTable_Find(HashTable* table,
                    HTHash_t hash,
                    HTKey_t key,
                    HTKeyValue_t* keyvalue) {
  // STEP 2: implement HashTable_Find.
  const size_t bucket = HashKeyToBucketNum(table, newkeyvalue.hash);
  LinkedList* chain = table->buckets[bucket];
  LinkedListNode* node = chain->head;
  while (node != nullptr) {
    if (table->key_cmp_fn(node->payload->key, key)) {
      *keyvalue = *reinterpret_cast<HTKeyValue_t*>(node->payload);
      return true;
    }
    node = node->next;
  }
  return false;  // you may need to change this return value
}

bool HashTable_Remove(HashTable* table,
                      HTHash_t hash,
                      HTKey_t key,
                      HTKeyValue_t* keyvalue) {
  // STEP 3: implement HashTable_Remove.
  const size_t bucket = HashKeyToBucketNum(table, hash);
  LinkedList* chain = table->buckets[bucket];
  LinkedListNode* node = chain->head;
  while (node != nullptr) {
    if (table->key_cmp_fn(node->payload->key, key)) {
      *keyvalue = *reinterpret_cast<HTKeyValue_t*>(node->payload);
      LinkedList_Remove(chain, node->payload);
      table->num_elements--;
      return true;
    }
    node = node->next;
  }
  return false;  // you may need to change this return value
}

///////////////////////////////////////////////////////////////////////////////
// HTIterator implementation.

// Implemented for you
HTIterator* HTIterator_New(HashTable* table) {
  HTIterator* iter = new HTIterator{};

  // If the hash table is empty, the iterator is immediately invalid,
  // since it can't point to anything.
  if (table->num_elements == 0 || table->num_buckets == 0) {
    iter->ht = table;
    iter->bucket_it = nullptr;
    iter->bucket_idx = k_invalid_index;
    return iter;
  }

  // Initialize the iterator.  There is at least one element in the
  // table, so find the first element and point the iterator at it.
  iter->ht = table;
  iter->bucket_idx = 0;
  for (int i = 0; i < table->num_buckets; i++) {
    if (LinkedList_NumElements(table->buckets[i]) > 0) {
      iter->bucket_idx = i;
      break;
    }
  }
  iter->bucket_it = LLIterator_New(table->buckets[iter->bucket_idx]);
  return iter;
}

// Implemented for you
void HTIterator_Delete(HTIterator* iter) {
  if (iter->bucket_it != nullptr) {
    LLIterator_Delete(iter->bucket_it);
    iter->bucket_it = nullptr;
  }
  delete iter;
}

bool HTIterator_IsValid(HTIterator* iter) {
  // STEP 4: implement HTIterator_IsValid.
  return iter->bucket_it != nullptr;
  return true;  // you may need to change this return value
}

bool HTIterator_Next(HTIterator* iter) {
  // STEP 5: implement HTIterator_Next.
  if (iter->bucket_it == nullptr) {
    return false;
  }
  if (LLIterator_Next(iter->bucket_it)) {
    return true;
  }
  iter->bucket_idx++;
  if (iter->bucket_idx >= iter->ht->num_buckets) {
    return false;
  }
  iter->bucket_it = LLIterator_New(iter->ht->buckets[iter->bucket_idx]);
  return true;  // you may need to change this return value
}

bool HTIterator_Get(HTIterator* iter, HTKeyValue_t* keyvalue) {
  // STEP 6: implement HTIterator_Get.
  if (iter->bucket_it == nullptr) {
    return false;
  }
  LLPayload_t payload;
  LLIterator_Get(iter->bucket_it, &payload);
  *keyvalue = *reinterpret_cast<HTKeyValue_t*>(payload);
  return true;
}

// Implemented for you
bool HTIterator_Remove(HTIterator* iter, HTKeyValue_t* keyvalue) {
  HTKeyValue_t kv;

  // Try to get what the iterator is pointing to.
  if (!HTIterator_Get(iter, &kv)) {
    return false;
  }

  // Advance the iterator.  Thanks to the above call to
  // HTIterator_Get, we know that this iterator is valid (though it
  // may not be valid after this call to HTIterator_Next).
  HTIterator_Next(iter);

  // Lastly, remove the element.  Again, we know this call will succeed
  // due to the successful HTIterator_Get above.
  HashTable_Remove(iter->ht, kv.hash, kv.key, keyvalue);

  return true;
}

// Implemented for you
static void MaybeResize(HashTable* ht) {
  // Resize if the load factor is > 3.
  if (ht->num_elements < 3 * ht->num_buckets) {
    return;
  }

  // This is the resize case.  Allocate a new hashtable,
  // iterate over the old hashtable, do the surgery on
  // the old hashtable record and deallocate the new hashtable
  // record.
  HashTable* newht = HashTable_New(ht->num_buckets * 9, ht->key_cmp_fn);

  // Loop through the old ht copying its elements over into the new one.
  HTIterator* it = HTIterator_New(ht);
  for (; HTIterator_IsValid(it); HTIterator_Next(it)) {
    HTKeyValue_t item, unused;

    HTIterator_Get(it, &item);
    HashTable_Insert(newht, item, &unused);
  }

  // Swap the new table onto the old, then deallocate the old table (tricky!).  We
  // use the "no-op free" because we don't actually want to deallocate the elements;
  // they're owned by the new table.
  const HashTable tmp = *ht;
  *ht = *newht;
  *newht = tmp;

  // Done!  Clean up our iterator and temporary table.
  HTIterator_Delete(it);
  HashTable_Delete(newht, &HTNoOpDelete);
}
