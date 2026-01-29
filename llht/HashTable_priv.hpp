#ifndef HASHTABLE_PRIV_HPP_
#define HASHTABLE_PRIV_HPP_

#include <cstdint>  // for uint32_t, etc.

#include "./HashTable.hpp"
#include "./LinkedList.hpp"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Internal structures and helper functions for our HashTable implementation.
//
// These would typically be located in HashTable.cpp; however, we have broken
// them out into a "private .hpp" so that our unittests can access them.  This
// allows our test code to peek inside the implementation to verify correctness.
//
// Customers should not include this file or assume anything based on
// its contents.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// The hash table implementation.
//
// A hash table is an array of buckets, where each bucket is a linked list
// of HTKeyValue structs.
typedef struct ht {
  size_t num_buckets;      // # of buckets in this HT
  size_t num_elements;     // # of elements currently in this HT
  LinkedList** buckets;    // the array of buckets
  KeyCmpFnPtr key_cmp_fn;  // to check for key collisions
} HashTable;

// The hash table iterator.
typedef struct ht_it {
  HashTable* ht;          // the HT we're pointing into
  size_t bucket_idx;      // which bucket are we in?
  LLIterator* bucket_it;  // iterator for the bucket, or nullptr
} HTIterator;

// This is the internal hash function we use to map from HTHash_t hashes to a
// bucket number.
size_t HashToBucketNum(HashTable* ht, HTHash_t hash);

#endif  // HASHTABLE_PRIV_HPP_
