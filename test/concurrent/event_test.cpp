#include "gtest/gtest.h"

#include "concurrent/event.h"
#include "concurrent/instr.h"

using namespace se;

/// Public constructor of superclass
class TestEvent : public Event {
public:
  TestEvent(const MemoryAddr& addr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(addr, condition_ptr) {}
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
  const TestEvent event(0x03fa);
  EXPECT_EQ(nullptr, event.condition_ptr());
}

TEST(EventTest, ConditionalEventConstructor) {
  Event::reset_id();
  uintptr_t condition_read_access = 0x03fa;

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  uintptr_t another_access = 0x03fb;

  const TestEvent another_event(another_access, condition_ptr);
  EXPECT_EQ(1, another_event.id());
  EXPECT_EQ(1, another_event.addr().ptrs().size());
  EXPECT_NE(nullptr, another_event.condition_ptr());
  EXPECT_EQ(condition_ptr, another_event.condition_ptr());
}

TEST(EventTest, WriteEventWithCondition) {
  Event::reset_id(5);
  uintptr_t condition_read_access = 0x03fa;

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  uintptr_t read_access = 0x03fb;

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));

  uintptr_t write_access = 0x03fc;

  const WriteEvent<long> write_event(write_access, std::move(read_instr_ptr), condition_ptr);
  const LiteralReadInstr<long>& read_instr = dynamic_cast<const LiteralReadInstr<long>&>(write_event.instr_ref());

  EXPECT_EQ(6, write_event.id());
  EXPECT_EQ(1, write_event.addr().ptrs().size());
  EXPECT_EQ(condition_ptr, write_event.condition_ptr());
  EXPECT_EQ(42L, read_instr.literal());
}

TEST(EventTest, WriteEventWithoutCondition) {
  Event::reset_id(5);

  uintptr_t read_access = 0x03fb;
  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  uintptr_t write_access = 0x03fc;

  const WriteEvent<long> write_event(write_access, std::move(read_instr_ptr));
  const LiteralReadInstr<long>& read_instr = dynamic_cast<const LiteralReadInstr<long>&>(write_event.instr_ref());

  EXPECT_EQ(5, write_event.id());
  EXPECT_EQ(1, write_event.addr().ptrs().size());
  EXPECT_EQ(nullptr, write_event.condition_ptr());
  EXPECT_EQ(42L, read_instr.literal());
}

TEST(EventTest, WriteEventWithoutInstr) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";

  uintptr_t write_access = 0x03fc;
  EXPECT_EXIT((WriteEvent<long>(write_access, nullptr)),
    ::testing::KilledBySignal(SIGABRT), "Assertion");
}

TEST(EventTest, ReadEventWithCondition) {
  Event::reset_id(5);
  uintptr_t condition_read_access = 0x03fa;

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  uintptr_t another_read_access = 0x03fb;

  const ReadEvent<int> another_event(another_read_access, condition_ptr);
  EXPECT_EQ(6, another_event.id());
  EXPECT_EQ(1, another_event.addr().ptrs().size());
  EXPECT_NE(nullptr, another_event.condition_ptr());
  EXPECT_EQ(condition_ptr, another_event.condition_ptr());
}

TEST(EventTest, ReadEventWithoutCondition) {
  Event::reset_id(5);

  uintptr_t another_read_access = 0x03fb;

  const ReadEvent<int> another_event(another_read_access);
  EXPECT_EQ(5, another_event.id());
  EXPECT_EQ(1, another_event.addr().ptrs().size());
  EXPECT_EQ(nullptr, another_event.condition_ptr());
}
