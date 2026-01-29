#include <cstddef>
#include <string>

#include "./HashTable.hpp"
#include "./HashTable_priv.hpp"
#include "./LinkedList.hpp"
#include "./LinkedList_priv.hpp"

#include "./catch.hpp"

using std::string;
using std::to_string;

// Our payload structure
typedef struct payload_st {
  int magic_num;
  int payload_num;
} Payload;

// statics:
static int g_free_invocations = 0;
static constexpr int k_magic_num = static_cast<int>(0xDEADBEEF);

static void VerifiedDelete(HTKeyValue_t payload) {
  string* key = static_cast<string*>(payload.key);
  Payload* value = static_cast<Payload*>(payload.value);
  REQUIRE(k_magic_num == value->magic_num);
  delete key;
  delete value;
}

static void InstrumentedDelete(HTKeyValue_t payload) {
  g_free_invocations++;
  VerifiedDelete(payload);
}

static bool CompareKeys(HTKey_t lhs, HTKey_t rhs) {
  string* lhs_str = static_cast<string*>(lhs);
  string* rhs_str = static_cast<string*>(rhs);
  return *lhs_str == *rhs_str;
}

static void NoOpDelete(HTKeyValue_t delete_me) {}

// listener to reset the g_free_invocations to 0 before every test
class HTTestSetupListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testCaseStarting(Catch::TestCaseInfo const&) override {
        g_free_invocations = 0;
    }
};

CATCH_REGISTER_LISTENER(HTTestSetupListener)

// test cases

TEST_CASE("New", "[Test_HashTable]") {
  HashTable* ht = HashTable_New(3, CompareKeys);
  REQUIRE(0 == ht->num_elements);
  REQUIRE(3 == ht->num_buckets);

  REQUIRE(ht->buckets != nullptr);
  REQUIRE(0 == LinkedList_NumElements(ht->buckets[0]));
  REQUIRE(0 == LinkedList_NumElements(ht->buckets[1]));
  REQUIRE(0 == LinkedList_NumElements(ht->buckets[2]));
  HashTable_Delete(ht, &VerifiedDelete);
}

TEST_CASE("InsertFindRemove", "[Test_HashTable]") {
  HashTable* table = HashTable_New(10, CompareKeys);
  int i;

  // Allocate and insert a bunch of elements.
  for (i = 0; i < 25; i++) {
    const HTHash_t hash = static_cast<HTHash_t>(i);
    HTKey_t key = static_cast<HTKey_t>(new string(to_string(i)));

    // Create an element and do the insert.  Note that we promptly overwrite
    // these elements in the next section, so we don't bother
    // allocating/freeing memory for these items.
    Payload* np;
    HTKeyValue_t oldkv{};
    HTKeyValue_t newkv{hash, key, static_cast<HTValue_t>(&newkv)};
    REQUIRE_FALSE(HashTable_Insert(table, newkv, &oldkv));

    // Test the double-insert case, using a different dynamically-allocated
    // value.  We compare the returned "old" element with the just-inserted
    // stack-allocated element, above.
    np = new Payload{k_magic_num, i};
    newkv.value = static_cast<HTValue_t>(np);
    REQUIRE(HashTable_Insert(table, newkv, &oldkv));
    REQUIRE(hash == oldkv.hash);
    REQUIRE(key == oldkv.key);
    REQUIRE(i == stoi(*static_cast<string*>(oldkv.key)));
    REQUIRE(static_cast<HTValue_t>(&newkv) == oldkv.value);

    // Lookup the newly-inserted value.
    oldkv.hash = -1;      // reinitialize "oldkv" so we can verify it was
    oldkv.key = nullptr;  // set by Find.
    oldkv.value = nullptr;
    REQUIRE(HashTable_Find(table, hash, key, &oldkv));
    REQUIRE(hash == oldkv.hash);
    REQUIRE(key == oldkv.key);
    REQUIRE(i == stoi(*static_cast<string*>(oldkv.key)));
    REQUIRE(static_cast<HTValue_t>(np) == oldkv.value);

    // Lookup and remove a value that doesn't exist in the table.
    REQUIRE_FALSE(HashTable_Find(table, hash + 1, nullptr, &oldkv));
    REQUIRE_FALSE(HashTable_Remove(table, hash + 1, nullptr, &oldkv));

    // Remove the item we just inserted.
    oldkv.hash = -1;
    oldkv.key = nullptr;
    oldkv.value = nullptr;
    REQUIRE(HashTable_Remove(table, hash, key, &oldkv));
    REQUIRE(hash == oldkv.hash);
    REQUIRE(key == oldkv.key);
    REQUIRE(i == stoi(*static_cast<string*>(oldkv.key)));
    REQUIRE(static_cast<HTValue_t>(np) == oldkv.value);
    REQUIRE(i == HashTable_NumElements(table));

    // Insert it again.
    REQUIRE_FALSE(HashTable_Insert(table, newkv, &oldkv));
    REQUIRE(HashTable_Insert(table, newkv, &oldkv));
    REQUIRE(i + 1 == HashTable_NumElements(table));
  }

  // Delete every other key.
  for (i = 0; i < 25; i += 2) {
    const HTHash_t hash = static_cast<HTHash_t>(i);
    HTKeyValue_t oldkv;

    string* key = new string(to_string(i));

    REQUIRE(HashTable_Remove(table, i, key, &oldkv));
    REQUIRE(hash == oldkv.hash);
    VerifiedDelete(oldkv);

    // This second attempt fails, since the element was already removed.
    REQUIRE_FALSE(HashTable_Remove(table, i, key, &oldkv));

    delete key;
  }

  REQUIRE(table->num_elements == 12);

  // Delete the remaining keys.
  for (i = 1; i < 25; i += 2) {
    const HTHash_t hash = static_cast<HTHash_t>(i);
    HTKeyValue_t oldkv;

    string* key = new string(to_string(i));

    REQUIRE(HashTable_Remove(table, hash, key, &oldkv));
    REQUIRE(hash == oldkv.hash);
    VerifiedDelete(oldkv);

    // As before, this second attempt should fail.
    REQUIRE_FALSE(HashTable_Remove(table, i, key, &oldkv));

    delete key;
  }
  REQUIRE(table->num_elements == 0);

  // One more pass, inserting elements.

  // insert some values that will have hash collisions but different keys
  // than ones inserted later
  HTHash_t hash = static_cast<HTHash_t>(0);
  string* key = new string("#DIV/0!");
  Payload* value = new Payload{k_magic_num, 0};
  HTKeyValue_t newkv{hash, key, value};
  HTKeyValue_t oldkv{};

  REQUIRE_FALSE(HashTable_Insert(table, newkv, &oldkv));

  // Ensure it's there.
  REQUIRE(HashTable_Find(table, hash, key, &oldkv));
  REQUIRE(oldkv.hash == hash);
  REQUIRE(oldkv.key == key);
  REQUIRE(oldkv.value == static_cast<HTValue_t>(value));

  // another

  hash = static_cast<HTHash_t>(16);
  key = new string("kinoue64");
  value = new Payload{k_magic_num, 16};
  newkv = {hash, key, value};

  REQUIRE_FALSE(HashTable_Insert(table, newkv, &oldkv));

  // Ensure it's there.
  REQUIRE(HashTable_Find(table, hash, key, &oldkv));
  REQUIRE(oldkv.hash == hash);
  REQUIRE(oldkv.key == key);
  REQUIRE(oldkv.value == static_cast<HTValue_t>(value));

  // one last possible collision insert
  hash = static_cast<HTHash_t>(16);
  key = new string("shar");
  value = new Payload{k_magic_num, 16};
  newkv = {hash, key, value};

  REQUIRE_FALSE(HashTable_Insert(table, newkv, &oldkv));

  // Ensure it's there.
  REQUIRE(HashTable_Find(table, hash, key, &oldkv));
  REQUIRE(oldkv.hash == hash);
  REQUIRE(oldkv.key == key);
  REQUIRE(oldkv.value == static_cast<HTValue_t>(value));

  for (i = 0; i < 25; i++) {
    const HTHash_t hash = static_cast<HTHash_t>(i);

    string* key = new string(to_string(i));

    // Do the insert.
    Payload* np = new Payload{k_magic_num, i};
    HTKeyValue_t oldkv{};
    const HTKeyValue_t newkv{hash, key, np};
    REQUIRE_FALSE(HashTable_Insert(table, newkv, &oldkv));

    // Ensure it's there.
    REQUIRE(HashTable_Find(table, hash, key, &oldkv));
    REQUIRE(oldkv.hash == hash);
    REQUIRE(oldkv.key == key);
    REQUIRE(oldkv.value == static_cast<HTValue_t>(np));
  }
  REQUIRE(28 == table->num_elements);

  // Delete most of the remaining keys.
  for (i = 0; i < 23; i++) {
    const HTHash_t hash = static_cast<HTHash_t>(i);
    HTKeyValue_t oldkv;

    string* key = new string(to_string(i));

    REQUIRE(HashTable_Remove(table, hash, key, &oldkv));
    REQUIRE(hash == oldkv.hash);
    VerifiedDelete(oldkv);

    REQUIRE_FALSE(HashTable_Remove(table, hash, key, &oldkv));

    delete key;
  }
  REQUIRE(5 == table->num_elements);

  // Delete the HT and the final keys
  HashTable_Delete(table, &InstrumentedDelete);
  REQUIRE(5 == g_free_invocations);
}

TEST_CASE("Iterator", "[Test_HashTable]") {
  HashTable* table = HashTable_New(300, CompareKeys);
  HTKeyValue_t oldkv{};

  // Test using an iterator on an empty table.
  HTIterator* it = HTIterator_New(table);
  REQUIRE_FALSE(HTIterator_IsValid(it));
  REQUIRE_FALSE(HTIterator_Get(it, &oldkv));
  HTIterator_Delete(it);

  // Allocate and insert a bunch of elements, then create an iterator for
  // the populated table.
  for (int i = 0; i < 100; i++) {
    const HTHash_t hash = static_cast<HTHash_t>(i);

    string* key = new string(to_string(i));

    // Create an element and do the insert.
    Payload* np = new Payload{k_magic_num, i};
    HTKeyValue_t oldkv{};
    const HTKeyValue_t newkv{hash, key, np};
    REQUIRE_FALSE(HashTable_Insert(table, newkv, &oldkv));
  }
  it = HTIterator_New(table);
  REQUIRE(HTIterator_IsValid(it));

  // Now iterate through the table, verifying each value is found exactly once.
  std::array<int, 100> num_times_seen = {0};  // array of 100 0's
  for (int i = 0; i < 100; i++) {
    Payload* op = nullptr;

    REQUIRE(HTIterator_IsValid(it));
    REQUIRE(HTIterator_Get(it, &oldkv));
    const int hash = static_cast<int>(oldkv.hash);

    // Verify that we've never seen this key before, then increment the
    // number of times we've seen it.
    REQUIRE(0 == num_times_seen.at(hash));
    num_times_seen.at(hash)++;

    // Verify that this is the value we previously inserted.
    op = static_cast<Payload*>(oldkv.value);
    REQUIRE(k_magic_num == op->magic_num);
    REQUIRE(hash == op->payload_num);

    // Increment the iterator.
    if (i == 99) {
      REQUIRE(HTIterator_IsValid(it));
      REQUIRE_FALSE(HTIterator_Next(it));
      REQUIRE_FALSE(HTIterator_IsValid(it));
    } else {
      REQUIRE(HTIterator_Next(it));
      REQUIRE(HTIterator_IsValid(it));
    }
  }
  for (int i = 0; i < 100; i++) {
    REQUIRE(1 == num_times_seen.at(i));  // verify each was seen exactly once.
  }

  REQUIRE_FALSE(HTIterator_Next(it));
  HTIterator_Delete(it);

  // Iterate through again, removing every third element and resetting all
  // the "was seen" counters.
  it = HTIterator_New(table);
  REQUIRE(HTIterator_IsValid(it));
  for (int i = 0; i < 100; i++) {
    REQUIRE(HTIterator_Get(it, &oldkv));
    const int htkey = static_cast<int>(oldkv.hash);
    num_times_seen[htkey] = 0;

    if (i % 3 == 0) {
      const size_t oldnumelements = HashTable_NumElements(table);
      Payload* op = static_cast<Payload*>(oldkv.value);
      const string* ok = static_cast<string*>(oldkv.key);
      REQUIRE(htkey == op->payload_num);
      num_times_seen.at(htkey)++;

      // Remove the element.  Don't forget that HTIterator_Remove automatically
      // increments the iterator.
      REQUIRE(HTIterator_Remove(it, &oldkv));
      REQUIRE(oldnumelements - 1 == HashTable_NumElements(table));
      delete op;
      delete ok;
    } else {
      // Manually increment the iterator.
      if (i == 99) {
        REQUIRE(HTIterator_IsValid(it));
        REQUIRE_FALSE(HTIterator_Next(it));
        REQUIRE_FALSE(HTIterator_IsValid(it));
      } else {
        REQUIRE(HTIterator_Next(it));
        REQUIRE(HTIterator_IsValid(it));
      }
    }
  }
  HTIterator_Delete(it);

  // Iterate through one last time, making sure we only retain elements whose
  // key is NOT a multiple of 3.
  it = HTIterator_New(table);
  REQUIRE(HTIterator_IsValid(it));

  REQUIRE(66 == HashTable_NumElements(table));
  for (int i = 0; i < 66; i++) {
    REQUIRE(HTIterator_Get(it, &oldkv));
    const int htkey = static_cast<int>(oldkv.hash);
    REQUIRE(0 == num_times_seen.at(htkey));

    if (i == 65) {
      REQUIRE(HTIterator_IsValid(it));
      REQUIRE_FALSE(HTIterator_Next(it));
      REQUIRE_FALSE(HTIterator_IsValid(it));
    } else {
      REQUIRE(HTIterator_Next(it));
      REQUIRE(HTIterator_IsValid(it));
    }
  }
  HTIterator_Delete(it);

  // Delete the HT and the final remaining keys.
  HashTable_Delete(table, &InstrumentedDelete);
  REQUIRE(66 == g_free_invocations);
}

TEST_CASE("Resize", "[Test_HashTable]") {
  HashTable* table = HashTable_New(2, CompareKeys);
  REQUIRE(2 == table->num_buckets);

  HTKeyValue_t newval;
  HTKeyValue_t oldkv;

  // Add elements to the Table, expect the table to resize
  // which makes use of HashTable_Iterator.
  for (int i = 0; i < 7; ++i) {
    newval.hash = i;
    newval.key = new string(to_string(i));
    newval.value = reinterpret_cast<HTValue_t>(static_cast<int64_t>(i));
    REQUIRE_FALSE(HashTable_Insert(table, newval, &oldkv));
    REQUIRE(HashTable_Insert(table, newval, &oldkv));
    REQUIRE(newval.hash == oldkv.hash);
    REQUIRE(newval.key == oldkv.key);
    REQUIRE(newval.value == oldkv.value);

    oldkv.hash = -1;      // reinitialize "oldkv" so we can verify it was
    oldkv.key = nullptr;  // set by HashTable_Find.
    oldkv.value = nullptr;
    REQUIRE(HashTable_Find(table, newval.hash, newval.key, &oldkv));
    REQUIRE(newval.hash == oldkv.hash);
    REQUIRE(newval.key == oldkv.key);
    REQUIRE(newval.value == oldkv.value);
  }

  REQUIRE(2 < table->num_buckets);
  const size_t old_buckets = table->num_buckets;

  // Make sure that all of the elements are still inside the
  // HashTable after Resizing, then ensure that num_buckets
  // stays the same.
  for (int i = 0; i < 7; ++i) {
    const HTHash_t hash = i;
    string* key = new string(to_string(i));
    HTValue_t value = reinterpret_cast<HTValue_t>(hash);

    oldkv.hash = -1;      // reinitialize "oldkv" so we can verify it was
    oldkv.key = nullptr;  // set by HashTable_Find.
    oldkv.value = nullptr;
    REQUIRE(HashTable_Find(table, hash, key, &oldkv));
    REQUIRE(hash == oldkv.hash);
    REQUIRE(CompareKeys(key, oldkv.key));
    REQUIRE(value == oldkv.value);

    oldkv.hash = -1;      // reinitialize "oldkv" so we can verify it was
    oldkv.key = nullptr;  // set by HashTable_Find.
    oldkv.value = nullptr;
    REQUIRE(HashTable_Remove(table, hash, key, &oldkv));
    REQUIRE(hash == oldkv.hash);
    REQUIRE(CompareKeys(key, oldkv.key));
    REQUIRE(value == oldkv.value);

    // Assert that the KeyValue is no longer within the HashTable
    REQUIRE_FALSE(HashTable_Find(table, hash, key, &oldkv));
    REQUIRE_FALSE(HashTable_Remove(table, hash, key, &oldkv));

    delete static_cast<string*>(oldkv.key);

    delete key;
  }

  // Assert that the number of buckets has not changed
  REQUIRE(2 < table->num_buckets);
  REQUIRE(old_buckets == table->num_buckets);

  HashTable_Delete(table, NoOpDelete);
}
