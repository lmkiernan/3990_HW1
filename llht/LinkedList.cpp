#include <cstdlib>

#include "LinkedList.hpp"
#include "LinkedList_priv.hpp"

///////////////////////////////////////////////////////////////////////////////
// LinkedList implementation.

LinkedList* LinkedList_New() {
  // TODO: allocate the LinkedList struct and initialize the newly allocated
  // record structure.
  LinkedList* list = new LinkedList();
  list->num_elements = 0;
  list->head = nullptr;
  list->tail = nullptr;
  return list;  // you may want to change this
}

void LinkedList_Delete(LinkedList* list,
                       LLPayloadFreeFnPtr payload_free_function) {
  // TODO: sweep through the list and deallocate all of the nodes' payloads
  // (using the payload_free_function supplied as an argument) and
  // the nodes themselves.
  // delete the LinkedList
  while (list->head != nullptr) {
    LinkedListNode* node = list->head;
    list->head = node->next;
    payload_free_function(node->payload);
    delete node;
  }
  list->tail = nullptr;
  list->num_elements = 0;
  delete list;
}

size_t LinkedList_NumElements(LinkedList* list) {
  return list->num_elements;
}

void LinkedList_Push(LinkedList* list, LLPayload_t payload) {
  // TODO: implement LinkedList_Push
  LinkedListNode* node = new LinkedListNode();
  node->payload = payload;
  node->next = list->head;
  node->prev = nullptr;
  if (list->head != nullptr) {
    list->head->prev = node;
  }
  list->head = node;
  if (list->tail == nullptr) {
    list->tail = node;
  }
  list->num_elements++;
}

bool LinkedList_Pop(LinkedList* list, LLPayload_t* payload_ptr) {
  // TODO: implement LinkedList_Pop.  Make sure you test for
  // and empty list and fail.  If the list is non-empty, there
  // are two cases to consider: (a) a list with a single element in it
  // and (b) the general case of a list with >=2 elements in it.
  // Be sure to call delete to deallocate the memory that was
  // previously allocated by LinkedList_Push().
  if (list->num_elements == 0) {
    return false;
  }

  LinkedListNode* node = list->head;
  *payload_ptr = node->payload;
  list->head = node->next;
  if (list->head != nullptr) {
    list->head->prev = nullptr;
  }
  delete node;
  list->num_elements--;
  return true;  // you may need to change this return value
}

void LinkedList_Append(LinkedList* list, LLPayload_t payload) {
  // TODO: implement LinkedList_Append.  It's kind of like
  // LinkedList_Push, but obviously you need to add to the end
  // instead of the beginning.
  LinkedListNode* node = new LinkedListNode();
  node->payload = payload;
  node->next = nullptr;
  node->prev = list->tail;
  if (list->tail != nullptr) {
    list->tail->next = node;
  }
  list->tail = node;
  if (list->head == nullptr) {
    list->head = node;
  }
  list->num_elements++;
}

bool LinkedList_Slice(LinkedList* list, LLPayload_t* payload_ptr) {
  // TODO: implement LinkedList_Slice.
  if (list->num_elements == 0) {
    return false;
  }
  LinkedListNode* node = list->tail;
  *payload_ptr = node->payload;
  list->tail = node->prev;
  if (list->tail != nullptr) {
    list->tail->next = nullptr;
  }
  delete node;
  list->num_elements--;
  return true;  // you may need to change this return value
}

///////////////////////////////////////////////////////////////////////////////
// LLIterator implementation.

LLIterator* LLIterator_New(LinkedList* list) {
  LLIterator* iter = new LLIterator();
  iter->list = list;
  iter->node = list->head;
  return iter; 
}

// implemented for you
void LLIterator_Delete(LLIterator* iter) {
  delete iter;
}

bool LLIterator_IsValid(LLIterator* iter) {
  // TODO: implement
  return iter->node != nullptr;
}

bool LLIterator_Next(LLIterator* iter) {
  // TODO: try to advance iterator to the next node and return true if
  // you succeed and are now on a new node, false otherwise
  if (iter->node == nullptr) {
    return false;
  }
  iter->node = iter->node->next;
  return true;
}

void LLIterator_Get(LLIterator* iter, LLPayload_t* payload) {
  if (iter->node == nullptr) {
    return;
  }
  *payload = iter->node->payload;
}

bool LLIterator_Remove(LLIterator* iter,
                       LLPayloadFreeFnPtr payload_free_function) {
  // TODO: implement LLIterator_Remove.  This is the most
  // complex function you'll build.  There are several cases
  // to consider:
  // - degenerate case: the list becomes empty after deleting.
  // - degenerate case: iter points at head
  // - degenerate case: iter points at tail
  // - fully general case: iter points in the middle of a list,
  //                       and you have to "splice".
  //
  // Be sure to call the payload_free_function to deallocate the payload
  // the iterator is pointing to, and also deallocate any LinkedList
  // data structure element as appropriate.
  if (iter->list->num_elements <= 1) {
    payload_free_function(iter->node->payload);
    delete iter->node;
    iter->list->head = nullptr;
    iter->list->tail = nullptr;
    iter->list->num_elements = 0;
    delete iter;
    return false;
  }
  if (iter->node.prev == nullptr) {
    iter->list->head = iter->node->next;
    iter->list->head->prev = nullptr;
    iter->list->num_elements--;
    payload_free_function(iter->node->payload);
    delete iter->node;
    iter->node = iter->list->head;
    return true;
  }
  if (iter->node->next == nullptr) {
    iter->list->tail = iter->node->prev;
    iter->list->tail->next = nullptr;
    iter->list->num_elements--;
    payload_free_function(iter->node->payload);
    delete iter->node;
    iter->node = iter->list->tail;
    return true;
  }
  iter->node->prev->next = iter->node->next;
  iter->node->next->prev = iter->node->prev;
  iter->list->num_elements--;
  payload_free_function(iter->node->payload);
  delete iter->node;
  iter->node = iter->node->next;
  return true;
}

// Implemented for you
void LLIterator_Rewind(LLIterator* iter) {
  iter->node = iter->list->head;
}
