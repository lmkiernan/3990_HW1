#ifndef LINKEDLIST_HPP_
#define LINKEDLIST_HPP_

#include <cstdint>  // for uint64_t, etc.
#include <cstddef>  // for size_t

///////////////////////////////////////////////////////////////////////////////
// A LinkedList is a doubly-linked list.
//
// We provide the interface to the LinkedList here; your job is to fill
// in the implementation holes that we left in LinkedList.cpp.

// Since C doesn't have proper objects that OOP languages like Java and C++
// have, this interface will look slightly odd for our "object".
// To hide the implementation of our LinkedList, we declare the "struct ll"
// structure and its associated typedef here, but we *define* the structure
// in the internal header LinkedList_priv.hpp. This lets us define a pointer
// to Deque as a new type while leaving the implementation details
// private to the customer.
typedef struct ll LinkedList;

// LLPayload type definition:
// We aren't using the templatized items like we do in C++ or Java,
// so we can generalize the values stored in the data structure by using
// a void* and then having the user cast data to a void* or have the void*
// point to dynamically allocated data on the heap. Note that the user is
// responsible for doing such allocation/de-allocation and is responsible
// for using the types stored in the list correctly.
typedef void* LLPayload_t;

// When we deallocate our LinkedList, there may be items still in the list.
// Since we want our list to be generalizable, we need the user to specify
// how to deallocate the elements that are still in the list, thus the
// function pointer definition we have below.
typedef void (*LLPayloadFreeFnPtr)(LLPayload_t payload);

// Allocate and return a new linked list.  The caller takes responsibility for
// eventually calling LinkedList_Delete to deallocate memory associated with the
// list.
//
// Arguments: none.
//
// Returns:
// - the newly-allocated linked list or nullptr on error.
LinkedList* LinkedList_New();

// Free a linked list that was previously allocated by LinkedList_New.
//
// Arguments:
// - list: the linked list to free.  It is unsafe to use "list" after this
//   function returns.
// - payload_free_function: a pointer to a payload freeing function; see above
//   for details on what this is.
void LinkedList_Delete(LinkedList* list,
                       LLPayloadFreeFnPtr payload_free_function);

// Return the number of elements in the linked list.
//
// Arguments:
// - list:  the list to query.
//
// Returns:
// - list length.
size_t LinkedList_NumElements(LinkedList* list);

// Adds a new element to the head of the linked list.
//
// Arguments:
// - list: the LinkedList to push onto.
// - payload: the payload to push; it's up to the caller to interpret and
//   manage the memory of the payload.
void LinkedList_Push(LinkedList* list, LLPayload_t payload);

// Pop an element from the head of the linked list.
//
// Arguments:
// - list: the LinkedList to pop from.
// - payload_ptr: a return parameter; on success, the popped node's payload
//   is returned through this parameter.
//
// Returns:
// - false on failure (eg, the list is empty).
// - true on success.
bool LinkedList_Pop(LinkedList* list, LLPayload_t* payload_ptr);

// Adds a new element to the tail of the linked list.
//
// This is the "tail" version of LinkedList_Push.
//
// Arguments:
// - list: the LinkedList to push onto.
// - payload: the payload to push; it's up to the caller to interpret and
//   manage the memory of the payload.
void LinkedList_Append(LinkedList* list, LLPayload_t payload);

// Remove an element from the tail of the linked list.
//
// This is the "tail" version of LinkedList_Pop, and the converse of
// LinkedList_Append.
//
// Arguments:
// - list: the LinkedList to remove from
// - payload_ptr: a return parameter; on success, the sliced node's payload
//   is returned through this parameter.
//
// Returns:
// - false: on failure (eg, the list is empty).
// - true: on success.
bool LinkedList_Slice(LinkedList* list, LLPayload_t* payload_ptr);

///////////////////////////////////////////////////////////////////////////////
// Linked list iterator.
//
// Linked lists support the notion of an iterator, similar to Java iterators.
// You use an iterator to navigate back and forth through the linked list and
// to insert/remove elements from the list.  You use LLIterator_New() to
// manufacture a new iterator and LLIterator_Delete() to free an iterator when
// you're done with it.
//
// If you use a LinkedList*() function to mutate a linked list, any iterators
// you have on that list become undefined (ie, dangerous to use; arbitrary
// memory corruption can occur). Thus, you should only use LLIterator*()
// functions in between the manufacturing and freeing of an iterator.
typedef struct ll_iter LLIterator;  // same trick to hide implementation.

// Manufacture an iterator for the list.  Caller is responsible for
// eventually calling LLIterator_Delete to free memory associated with
// the iterator.
//
// Arguments:
// - list: the list from which we'll return an iterator.
//
// Returns:
// - a newly-allocated iterator, which may be invalid or "past the end" if
//   the list cannot be iterated through (eg, empty).
LLIterator* LLIterator_New(LinkedList* list);

// When you're done with an iterator, you must free it by calling this
// function.
//
// Arguments:
// - iter: the iterator to free. Don't use it after freeing it.
void LLIterator_Delete(LLIterator* iter);

// Tests to see whether the iterator is pointing at a valid element.
//
// Arguments:
// - iter: the iterator to test.
//
// Returns:
// - true: if iter is not past the end of the list.
// - false: if iter is past the end of the list.
bool LLIterator_IsValid(LLIterator* iter);

// Advance the iterator, i.e. move to the next node in the list.  The
// passed-in iterator must be valid (eg, not "past the end").
//
// Arguments:
// - iter: the iterator.
//
// Returns:
// - true: if the iterator has been advanced to the next node.
// - false: if the iterator is no longer valid after the
//   advancing has completed (eg, it's now "past the end").
bool LLIterator_Next(LLIterator* iter);

// Returns the payload of the list node that the iterator currently points
// at.  The passed-in iterator must be valid (eg, not "past the end").
//
// Arguments:
// - iter: the iterator to fetch the payload from.
// - payload: a "return parameter" through which the payload is returned.
void LLIterator_Get(LLIterator* iter, LLPayload_t* payload);

// Remove the node the iterator is pointing to.  After deletion, the iterator
// may be in one of the following three states:
// - if there was only one element in the list, the iterator is now invalid
//   and cannot be used.  In this case, the caller is recommended to free
//   the now-invalid iterator.
// - if the deleted node had a successor (ie, it was not pointing at the tail)
//   the iterator is now pointing at the successor.
// - if the deleted node was the tail, the iterator is now pointing at the
//    predecessor.
//
// The passed-in iterator must be valid (eg, not "past the end").
//
// Arguments:
// - iter:  the iterator to delete from.
// - payload_free_function: invoked to free the payload.
//
// Returns:
// - false if the deletion succeeded, but the list is now empty.
// - true if the deletion succeeded, and the list is still non-empty.
bool LLIterator_Remove(LLIterator* iter,
                       LLPayloadFreeFnPtr payload_free_function);

// Rewind an iterator to the front of its list.
//
// Arguments:
// - iter: the iterator to rewind.
void LLIterator_Rewind(LLIterator* iter);

#endif  // LINKEDLIST_HPP_
