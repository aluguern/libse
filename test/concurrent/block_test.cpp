#include "gtest/gtest.h"

#include "concurrent/block.h"
#include "concurrent/encoder.h"

using namespace se;

class TestEvent : public Event {
public:
  TestEvent(size_t event_id) :
    Event(event_id, 0, MemoryAddr::alloc<int>(), true, &TypeInfo<int>::s_type) {}

  z3::expr encode_eq(const Z3ValueEncoder& encoder, Z3& helper) const {
    return helper.constant(*this);
  }

  z3::expr constant(Z3& helper) const { return helper.constant(*this); }
};

TEST(BlockTest, InsertEvents) {
  std::unique_ptr<Block> root_ptr(Block::make_root());
  EXPECT_NE(nullptr, root_ptr);

  const std::shared_ptr<Event> a(new TestEvent(1));
  const std::shared_ptr<Event> b(new TestEvent(2));
  const std::shared_ptr<Event> c(new TestEvent(3));
  const std::shared_ptr<Event> d(new TestEvent(4));

  std::forward_list<std::shared_ptr<Event>> event_ptrs;
  event_ptrs.push_front(c);
  event_ptrs.push_front(b);

  root_ptr->insert_event_ptr(a);
  root_ptr->insert_all(event_ptrs);
  root_ptr->insert_event_ptr(d);

  std::forward_list<std::shared_ptr<Event>> body(root_ptr->body());
  EXPECT_EQ(a, body.front());
  body.pop_front();

  EXPECT_EQ(b, body.front());
  body.pop_front();

  EXPECT_EQ(c, body.front());
  body.pop_front();

  EXPECT_EQ(d, body.front());
  body.pop_front();

  EXPECT_TRUE(body.empty());
}
