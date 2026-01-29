#include <unistd.h>
#include <bit>
#include <cerrno>

#include "./LinkedList.hpp"
#include "./LinkedList_priv.hpp"

#include "./catch.hpp"

// statics:
static const LLPayload_t k_one =
    std::bit_cast<LLPayload_t>(static_cast<uint64_t>(1));
static const LLPayload_t k_two =
    std::bit_cast<LLPayload_t>(static_cast<uint64_t>(2));
static const LLPayload_t k_three =
    std::bit_cast<LLPayload_t>(static_cast<uint64_t>(3));
static const LLPayload_t k_four =
    std::bit_cast<LLPayload_t>(static_cast<uint64_t>(4));
static const LLPayload_t k_five =
    std::bit_cast<LLPayload_t>(static_cast<uint64_t>(5));
static int g_free_invocations = 0;

static void StubbedDelete(LLPayload_t payload) {
  // Do nothing but verify the payload is non-nullptr and
  // increment the free count.
  REQUIRE(payload != nullptr);
  g_free_invocations++;
}

// listener to reset the g_free_invocations to 0 before every test
class LLTestSetupListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testCaseStarting(Catch::TestCaseInfo const&) override {
        g_free_invocations = 0;
    }
};

CATCH_REGISTER_LISTENER(LLTestSetupListener)

// test cases

TEST_CASE("Basic", "[Test_LinkedList]") {
  // Try creating a list.
  LinkedList* llp = LinkedList_New();
  REQUIRE(llp != nullptr);
  REQUIRE(0 == LinkedList_NumElements(llp));
  REQUIRE(nullptr == llp->head);
  REQUIRE(nullptr == llp->tail);

  // Try deleting the (empty) list.
  LinkedList_Delete(llp, &StubbedDelete);
  REQUIRE(0 == g_free_invocations);
  llp = nullptr;
}

TEST_CASE("PushPop", "[Test_LinkedList]") {
  // Creating a list.
  LinkedList* llp = LinkedList_New();
  REQUIRE(llp != nullptr);
  REQUIRE(0 == LinkedList_NumElements(llp));
  REQUIRE(nullptr == llp->head);
  REQUIRE(nullptr == llp->tail);

  // Insert an element.
  LinkedList_Push(llp, k_one);
  REQUIRE(1 == LinkedList_NumElements(llp));
  REQUIRE(llp->head == llp->tail);
  REQUIRE(nullptr == llp->head->prev);
  REQUIRE(nullptr == llp->tail->next);
  REQUIRE(k_one == llp->head->payload);

  // Pop the element.
  LLPayload_t payload_ptr;
  REQUIRE(LinkedList_Pop(llp, &payload_ptr));
  REQUIRE(k_one == payload_ptr);
  REQUIRE(0 == LinkedList_NumElements(llp));

  // Try (and fail) to pop the element a second time.
  REQUIRE_FALSE(LinkedList_Pop(llp, &payload_ptr));

  // Insert two elements.
  LinkedList_Push(llp, k_one);
  REQUIRE(1 == LinkedList_NumElements(llp));
  REQUIRE(llp->head == llp->tail);
  REQUIRE(nullptr == llp->head->prev);
  REQUIRE(nullptr == llp->tail->next);
  REQUIRE(k_one == llp->head->payload);

  LinkedList_Push(llp, k_two);
  REQUIRE(2 == LinkedList_NumElements(llp));
  REQUIRE(llp->head != llp->tail);
  REQUIRE(nullptr == llp->head->prev);
  REQUIRE(nullptr == llp->tail->next);
  REQUIRE(llp->tail == llp->head->next);
  REQUIRE(llp->head == llp->tail->prev);
  REQUIRE(k_two == llp->head->payload);
  REQUIRE(k_one == llp->tail->payload);

  // Pop the first element.
  REQUIRE(LinkedList_Pop(llp, &payload_ptr));
  REQUIRE(k_two == payload_ptr);
  REQUIRE(1 == LinkedList_NumElements(llp));
  REQUIRE(llp->head == llp->tail);
  REQUIRE(nullptr == llp->head->prev);
  REQUIRE(nullptr == llp->tail->next);
  REQUIRE(k_one == llp->head->payload);

  // Free the non-empty list.
  LinkedList_Delete(llp, &StubbedDelete);
  REQUIRE(1 == g_free_invocations);
  llp = nullptr;
}

TEST_CASE("AppendSlice", "[Test_LinkedList]") {
  // Creating a list.
  LinkedList* llp = LinkedList_New();
  REQUIRE(llp != nullptr);
  REQUIRE(0 == LinkedList_NumElements(llp));
  REQUIRE(nullptr == llp->head);
  REQUIRE(nullptr == llp->tail);

  // Insert an element.
  LinkedList_Append(llp, k_one);
  REQUIRE(1 == LinkedList_NumElements(llp));
  REQUIRE(llp->head == llp->tail);
  REQUIRE(nullptr == llp->head->prev);
  REQUIRE(nullptr == llp->tail->next);
  REQUIRE(k_one == llp->head->payload);

  // Delete the element.
  LLPayload_t payload_ptr;
  REQUIRE(LinkedList_Slice(llp, &payload_ptr));
  REQUIRE(k_one == payload_ptr);
  REQUIRE(0 == LinkedList_NumElements(llp));

  // Delete the element a second time.
  REQUIRE_FALSE(LinkedList_Slice(llp, &payload_ptr));

  // Insert two elements.
  LinkedList_Append(llp, k_one);
  REQUIRE(1 == LinkedList_NumElements(llp));
  REQUIRE(llp->head == llp->tail);
  REQUIRE(nullptr == llp->head->prev);
  REQUIRE(nullptr == llp->tail->next);
  REQUIRE(k_one == llp->head->payload);

  LinkedList_Append(llp, k_two);
  REQUIRE(2 == LinkedList_NumElements(llp));
  REQUIRE(llp->head != llp->tail);
  REQUIRE(nullptr == llp->head->prev);
  REQUIRE(nullptr == llp->tail->next);
  REQUIRE(llp->tail == llp->head->next);
  REQUIRE(llp->head == llp->tail->prev);
  REQUIRE(k_one == llp->head->payload);
  REQUIRE(k_two == llp->tail->payload);

  // Delete the first element.
  REQUIRE(LinkedList_Slice(llp, &payload_ptr));
  REQUIRE(k_two == payload_ptr);
  REQUIRE(1 == LinkedList_NumElements(llp));
  REQUIRE(llp->head == llp->tail);
  REQUIRE(nullptr == llp->head->prev);
  REQUIRE(nullptr == llp->tail->next);
  REQUIRE(k_one == llp->head->payload);

  // Delete the non-empty list.
  LinkedList_Delete(llp, &StubbedDelete);
  REQUIRE(1 == g_free_invocations);
  llp = nullptr;
}

TEST_CASE("Iterator", "[Test_LinkedList]") {
  // Create a linked list.
  LinkedList* llp = LinkedList_New();

  // Add some data to the list.
  LinkedList_Append(llp, k_five);
  LinkedList_Append(llp, k_four);
  LinkedList_Append(llp, k_three);
  LinkedList_Append(llp, k_two);
  LinkedList_Append(llp, k_one);

  // Create the iterator.
  LLIterator* lli = LLIterator_New(llp);
  REQUIRE(lli != nullptr);
  REQUIRE(llp == lli->list);
  REQUIRE(llp->head == lli->node);

  // Navigate using the iterator.
  LLPayload_t payload;
  LLIterator_Get(lli, &payload);
  REQUIRE(k_five == payload);
  REQUIRE(LLIterator_Next(lli));
  LLIterator_Get(lli, &payload);
  REQUIRE(k_four == payload);
  REQUIRE(LLIterator_Next(lli));
  LLIterator_Get(lli, &payload);
  REQUIRE(k_three == payload);
  REQUIRE(LLIterator_Next(lli));
  LLIterator_Get(lli, &payload);
  REQUIRE(k_two == payload);
  REQUIRE(LLIterator_Next(lli));
  LLIterator_Get(lli, &payload);
  REQUIRE(k_one == payload);
  REQUIRE_FALSE(LLIterator_Next(lli));
  REQUIRE_FALSE(LLIterator_IsValid(lli));

  // The list contains 5 elements; try a delete from the front of the list.
  LLIterator_Rewind(lli);
  LinkedListNode* prev = nullptr;
  LinkedListNode* next = lli->node->next;
  LinkedListNode* nextnext = lli->node->next->next;
  REQUIRE(LLIterator_Remove(lli, &StubbedDelete));
  REQUIRE(next == llp->head);
  REQUIRE(next == lli->node);
  REQUIRE(nullptr == lli->node->prev);
  REQUIRE(nextnext == lli->node->next);
  REQUIRE(4 == LinkedList_NumElements(lli->list));
  REQUIRE(1 == g_free_invocations);

  LLIterator_Get(lli, &payload);
  REQUIRE(k_four == payload);

  // Delete the rest, but move the iterator forward by one to test removing
  // from the middle (ie, the 2nd element).
  REQUIRE(LLIterator_Next(lli));
  prev = lli->node->prev;
  next = lli->node->next;
  nextnext = lli->node->next->next;
  REQUIRE(LLIterator_Remove(lli, &StubbedDelete));
  REQUIRE(3 == LinkedList_NumElements(lli->list));
  REQUIRE(next == lli->node);
  REQUIRE(prev == lli->node->prev);
  REQUIRE(nextnext == lli->node->next);
  REQUIRE(prev->next == lli->node);
  REQUIRE(lli->node == nextnext->prev);

  // We're still deleting from the middle (ie, the 2nd element).
  next = lli->node->next;
  prev = lli->node->prev;
  REQUIRE(LLIterator_Remove(lli, &StubbedDelete));
  REQUIRE(2 == LinkedList_NumElements(lli->list));
  REQUIRE(next == lli->node);
  REQUIRE(prev == lli->node->prev);
  REQUIRE(nullptr == lli->node->next);
  REQUIRE(prev->next == lli->node);

  // This deletes from the tail position, since we are at the second element of
  // a two-element list.
  prev = lli->node->prev;
  REQUIRE(LLIterator_Remove(lli, &StubbedDelete));
  REQUIRE(1 == LinkedList_NumElements(lli->list));
  REQUIRE(nullptr == lli->node->next);
  REQUIRE(prev == lli->node);
  REQUIRE(nullptr == lli->node->prev);
  REQUIRE(prev == llp->tail);  // edge case found 17sp

  // Remove the remaining node from the list.
  REQUIRE_FALSE(LLIterator_Remove(lli, &StubbedDelete));
  REQUIRE(0 == LinkedList_NumElements(lli->list));
  REQUIRE(nullptr == lli->node);
  REQUIRE(nullptr == llp->head);
  REQUIRE(nullptr == llp->tail);
  REQUIRE(5 == g_free_invocations);

  // Free the iterator.
  LLIterator_Delete(lli);

  // Free the list.
  LinkedList_Delete(llp, &StubbedDelete);
}
