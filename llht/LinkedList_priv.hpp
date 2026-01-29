#ifndef LINKEDLIST_PRIV_HPP_
#define LINKEDLIST_PRIV_HPP_

#include "./LinkedList.hpp"  // for LinkedList and LLIterator

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Internal structures and helper functions for our LinkedList implementation.
//
// These would typically be located in LinkedList.cpp; however, we have broken
// them out into a "private .hpp" so that our unittests can access them.  This
// allows our test code to peek inside the implementation to verify correctness.
//
// Customers should not include this file or assume anything based on
// its contents.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// A single node within a linked list.
//
// A node contains next and prev pointers as well as a customer-supplied
// payload pointer.
typedef struct ll_node {
  LLPayload_t payload;   // customer-supplied payload pointer
  struct ll_node* next;  // next node in list, or nullptr
  struct ll_node* prev;  // prev node in list, or nullptr
} LinkedListNode;

// The entire linked list.
//
// We provided a struct declaration (but not definition) in LinkedList.hpp;
// this is the associated definition.  This struct contains metadata
// about the linked list.
typedef struct ll {
  size_t num_elements;   //  # elements in the list
  LinkedListNode* head;  // head of linked list, or nullptr if empty
  LinkedListNode* tail;  // tail of linked list, or nullptr if empty
} LinkedList;

// A linked list iterator.
//
// We expose the struct declaration in LinkedList.hpp, but not the definition,
// similar to what we did above for the linked list itself.
typedef struct ll_iter {
  LinkedList* list;      // the list we're for
  LinkedListNode* node;  // the node we are at, or nullptr if broken
} LLIterator;

#endif  // LINKEDLIST_PRIV_HPP_
