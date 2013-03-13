#include "gtest/gtest.h"

#include "concurrent/event.h"
#include "concurrent/instr.h"
#include "concurrent/encoder.h"

using namespace se;

/// A read event on an integer in the main thread
class TestEvent : public Event {
public:
  TestEvent(const MemoryAddr& addr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(0, addr, true, &TypeInfo<int>::s_type, condition_ptr) {}

  TestEvent(unsigned thread_id, const MemoryAddr& addr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
        Event(thread_id, addr, true, &TypeInfo<int>::s_type, condition_ptr) {}
};

TEST(EventTest, EventId) {
  Event::reset_id(42);

  const MemoryAddr addr = MemoryAddr::alloc<int>();
  const TestEvent event(addr);

  EXPECT_EQ(2*42, event.event_id());
  Event::reset_id();
}

TEST(EventTest, ThreadId) {
  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  const TestEvent event(thread_id, addr, nullptr);
  EXPECT_EQ(thread_id, event.thread_id());
}

TEST(EventTest, EventEquality) {
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  const TestEvent event_a(addr);
  const TestEvent event_b(addr);

  EXPECT_TRUE(event_a == event_a);
  EXPECT_FALSE(event_a == event_b);
  EXPECT_FALSE(event_b == event_a);
}

TEST(EventTest, UnconditionalEventConstructor) {
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  const TestEvent event(addr);
  EXPECT_EQ(nullptr, event.condition_ptr());
}

TEST(EventTest, ConditionalEventConstructor) {
  Event::reset_id();

  const unsigned thread_id = 3;
  const MemoryAddr condition_addr = MemoryAddr::alloc<bool>();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_addr));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  const MemoryAddr another_addr = MemoryAddr::alloc<int>();
  const TestEvent another_event(another_addr, condition_ptr);

  EXPECT_EQ(2*1, another_event.event_id());
  EXPECT_NE(nullptr, another_event.condition_ptr());
  EXPECT_EQ(condition_ptr, another_event.condition_ptr());
}

TEST(EventTest, WriteEventWithCondition) {
  Event::reset_id(5);

  const unsigned thread_id = 3;
  const MemoryAddr condition_addr = MemoryAddr::alloc<bool>();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_addr));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));

  const MemoryAddr write_addr = MemoryAddr::alloc<long>();
  const DirectWriteEvent<long> write_event(thread_id, write_addr, std::move(read_instr_ptr), condition_ptr);
  const LiteralReadInstr<long>& read_instr = dynamic_cast<const LiteralReadInstr<long>&>(write_event.instr_ref());

  EXPECT_TRUE(write_event.is_write());
  EXPECT_FALSE(write_event.is_read());

  EXPECT_EQ(thread_id, write_event.thread_id());
  EXPECT_EQ(2*6+1, write_event.event_id());
  EXPECT_EQ(condition_ptr, write_event.condition_ptr());
  EXPECT_EQ(42L, read_instr.literal());
}

TEST(EventTest, WriteEventWithoutCondition) {
  Event::reset_id(5);

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));

  const unsigned thread_id = 3;
  const MemoryAddr write_addr = MemoryAddr::alloc<long>();
  const DirectWriteEvent<long> write_event(thread_id, write_addr, std::move(read_instr_ptr));
  const LiteralReadInstr<long>& read_instr = dynamic_cast<const LiteralReadInstr<long>&>(write_event.instr_ref());

  EXPECT_TRUE(write_event.is_write());
  EXPECT_FALSE(write_event.is_read());

  EXPECT_EQ(thread_id, write_event.thread_id());
  EXPECT_EQ(2*5+1, write_event.event_id());
  EXPECT_EQ(nullptr, write_event.condition_ptr());
  EXPECT_EQ(42L, read_instr.literal());
}

TEST(EventTest, WriteEventWithoutInstr) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";

  const unsigned thread_id = 3;
  const MemoryAddr write_addr = MemoryAddr::alloc<long>();

  EXPECT_EXIT((DirectWriteEvent<long>(thread_id, write_addr, nullptr)),
    ::testing::KilledBySignal(SIGABRT), "Assertion");
}

TEST(EventTest, ReadEventWithCondition) {
  Event::reset_id(5);

  const unsigned thread_id = 3;
  const MemoryAddr condition_addr = MemoryAddr::alloc<bool>();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_addr));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  const MemoryAddr another_addr = MemoryAddr::alloc<int>();
  const ReadEvent<int> another_event(thread_id, another_addr, condition_ptr);

  EXPECT_FALSE(another_event.is_write());
  EXPECT_TRUE(another_event.is_read());
  EXPECT_EQ(thread_id, another_event.thread_id());
  EXPECT_EQ(2*6, another_event.event_id());
  EXPECT_NE(nullptr, another_event.condition_ptr());
  EXPECT_EQ(condition_ptr, another_event.condition_ptr());
}

TEST(EventTest, ReadEventWithoutCondition) {
  Event::reset_id(5);

  const unsigned thread_id = 3;
  const MemoryAddr another_addr = MemoryAddr::alloc<int>();
  const ReadEvent<int> another_event(thread_id, another_addr);

  EXPECT_FALSE(another_event.is_write());
  EXPECT_TRUE(another_event.is_read());
  EXPECT_EQ(2*5, another_event.event_id());
  EXPECT_EQ(thread_id, another_event.thread_id());
  EXPECT_EQ(nullptr, another_event.condition_ptr());
}
