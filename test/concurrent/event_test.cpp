#include "gtest/gtest.h"

#include "concurrent/event.h"
#include "concurrent/instr.h"

using namespace se;

class TestEvent : public Event {
public:
  TestEvent(const MemoryAddr& addr) : Event(addr) {}
};

TEST(EventTest, EventId) {
  Event::reset_id(42);
  uintptr_t ptr = 0x03fa;

  const TestEvent event(ptr);

  EXPECT_EQ(42, event.id());
  Event::reset_id();
}

TEST(EventTest, EventEquality) {
  uintptr_t ptr = 0x03fa;

  const TestEvent event_a(ptr);
  const TestEvent event_b(ptr);

  EXPECT_TRUE(event_a == event_a);
  EXPECT_FALSE(event_a == event_b);
  EXPECT_FALSE(event_b == event_a);
}

TEST(EventTest, UnconditionalEventConstructor) {
  const Event event(0x03fa);
  EXPECT_EQ(nullptr, event.condition_ptr());
}

TEST(EventTest, ConditionalEventConstructor) {
  Event::reset_id();
  uintptr_t condition_read_access = 0x03fa;

  std::unique_ptr<Event> condition_event_ptr(new Event(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  uintptr_t another_access = 0x03fb;

  const Event another_event(another_access, condition_ptr);
  EXPECT_EQ(1, another_event.id());
  EXPECT_EQ(1, another_event.addr().ptrs().size());
  EXPECT_NE(nullptr, another_event.condition_ptr());
  EXPECT_EQ(condition_ptr, another_event.condition_ptr());
}

TEST(EventTest, WriteEvent) {
  Event::reset_id(5);
  uintptr_t condition_read_access = 0x03fa;

  std::unique_ptr<Event> condition_event_ptr(new Event(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  uintptr_t read_access = 0x03fb;

  std::unique_ptr<Event> read_event_ptr(new Event(read_access, condition_ptr));
  const std::shared_ptr<ReadInstr<int>> read_instr_ptr(
    new BasicReadInstr<int>(std::move(read_event_ptr)));

  uintptr_t write_access = 0x03fc;

  const WriteEvent<int> write_event(write_access, read_instr_ptr, condition_ptr);

  EXPECT_EQ(7, write_event.id());
  EXPECT_EQ(1, write_event.addr().ptrs().size());
  EXPECT_EQ(condition_ptr, write_event.condition_ptr());
}

TEST(EventTest, ReadEvent) {
  Event::reset_id(5);
  uintptr_t condition_read_access = 0x03fa;

  std::unique_ptr<Event> condition_event_ptr(new Event(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  uintptr_t another_read_access = 0x03fb;

  const ReadEvent<int> another_event(another_read_access, condition_ptr);
  EXPECT_EQ(6, another_event.id());
  EXPECT_EQ(1, another_event.addr().ptrs().size());
  EXPECT_NE(nullptr, another_event.condition_ptr());
  EXPECT_EQ(condition_ptr, another_event.condition_ptr());
}
