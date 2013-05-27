#include "gtest/gtest.h"

#include "concurrent/slice.h"
#include "concurrent/encoder_c0.h"

using namespace se;

class TestEvent : public Event {
public:
  TestEvent(unsigned event_id) :
    Event(event_id, 0, Zone::unique_atom(), true, &TypeInfo<int>::s_type) {}

  z3::expr encode_eq(const Z3ValueEncoderC0& encoder, Z3C0& helper) const {
    return helper.constant(*this);
  }

  z3::expr constant(Z3C0& helper) const { return helper.constant(*this); }
};

TEST(SliceTest, Append) {
  constexpr ThreadId thread_id = 7;
  Slice slice;

  const std::shared_ptr<Event> a(new TestEvent(1));
  const std::shared_ptr<Event> b(new TestEvent(2));
  const std::shared_ptr<Event> c(new TestEvent(3));
  const std::shared_ptr<Event> d(new TestEvent(4));
  const std::shared_ptr<Event> e(new TestEvent(5));

  std::forward_list<std::shared_ptr<Event>> event_ptrs;
  event_ptrs.push_front(c);
  event_ptrs.push_front(b);

  slice.append(thread_id, a);
  slice.append_all(thread_id, event_ptrs);
  slice.append(thread_id, d);

  slice.append(thread_id + 1, e);

  Slice::EventPtrsMap event_ptrs_map(slice.event_ptrs_map());
  Slice::EventPtrsMap::mapped_type& thread_event_ptrs = event_ptrs_map[thread_id];

  EXPECT_EQ(a, thread_event_ptrs.front());
  thread_event_ptrs.pop_front();

  EXPECT_EQ(b, thread_event_ptrs.front());
  thread_event_ptrs.pop_front();

  EXPECT_EQ(c, thread_event_ptrs.front());
  thread_event_ptrs.pop_front();

  EXPECT_EQ(d, thread_event_ptrs.front());
  thread_event_ptrs.pop_front();

  EXPECT_TRUE(thread_event_ptrs.empty());
}

TEST(SliceTest, AppendAllEmpty) {
  constexpr ThreadId thread_id = 7;
  Slice slice;

  EXPECT_TRUE(slice.event_ptrs_map().empty());

  std::forward_list<std::shared_ptr<Event>> event_ptrs;
  slice.append_all(thread_id, event_ptrs);

  EXPECT_FALSE(slice.event_ptrs_map().empty());

  const Slice::EventPtrsMap::mapped_type& thread_event_ptrs = slice.event_ptrs_map().at(thread_id);
  EXPECT_TRUE(thread_event_ptrs.empty());
}
