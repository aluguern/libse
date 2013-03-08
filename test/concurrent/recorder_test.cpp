#include "gtest/gtest.h"

#include "concurrent/recorder.h"

using namespace se;

TEST(RecorderTest, EmptyPathCondition) {
  PathCondition path_condition;
  EXPECT_EQ(nullptr, path_condition.top());
}

TEST(RecorderTest, NonemptyPathCondition) {
  std::unique_ptr<ReadInstr<bool>> true_condition(
    new LiteralReadInstr<bool>(true, nullptr));

  std::unique_ptr<ReadInstr<bool>> false_condition(
    new LiteralReadInstr<bool>(false, nullptr));

  PathCondition path_condition;
  path_condition.push(std::move(true_condition));
  path_condition.push(std::move(false_condition));

  const LiteralReadInstr<bool>& false_read_instr =
    dynamic_cast<const LiteralReadInstr<bool>&>(*path_condition.top());
  EXPECT_FALSE(false_read_instr.literal());

  path_condition.pop();

  const LiteralReadInstr<bool>& true_read_instr =
    dynamic_cast<const LiteralReadInstr<bool>&>(*path_condition.top());
  EXPECT_TRUE(true_read_instr.literal());

  path_condition.pop();

  EXPECT_EQ(nullptr, path_condition.top());
}

TEST(RecorderTest, RecordEvents) {
  Event::reset_id(3);

  unsigned thread_id = 3;

  uintptr_t char_access = 0x03fa;
  std::unique_ptr<ReadEvent<char>> char_event_ptr(new ReadEvent<char>(thread_id, char_access));
  std::unique_ptr<ReadInstr<char>> char_instr_ptr(new BasicReadInstr<char>(std::move(char_event_ptr)));

  uintptr_t long_access = 0x03fb;
  std::unique_ptr<ReadEvent<long>> long_event_ptr(new ReadEvent<long>(thread_id, long_access));
  std::unique_ptr<ReadInstr<long>> long_instr_ptr(new BasicReadInstr<long>(std::move(long_event_ptr)));

  std::unique_ptr<ReadInstr<long>> instr_ptr(
    new BinaryReadInstr<ADD, char, long>(std::move(char_instr_ptr), std::move(long_instr_ptr)));

  uintptr_t last_access = 0x03fc;
  const MemoryAddr write_addr(last_access);

  Recorder recorder(thread_id);
  recorder.instr(write_addr, std::move(instr_ptr));

  std::forward_list<std::shared_ptr<Event>>& event_ptrs = recorder.event_ptrs();

  const WriteEvent<long>& write_long_event = dynamic_cast<const WriteEvent<long>&>(*event_ptrs.front());
  EXPECT_EQ(2*5+1, write_long_event.id());

  event_ptrs.pop_front();

  const ReadEvent<long>& read_long_event = dynamic_cast<const ReadEvent<long>&>(*event_ptrs.front());
  EXPECT_EQ(2*4, read_long_event.id());

  event_ptrs.pop_front();

  const ReadEvent<char>& read_char_event = dynamic_cast<const ReadEvent<char>&>(*event_ptrs.front());
  EXPECT_EQ(2*3, read_char_event.id());

  event_ptrs.pop_front();
  EXPECT_TRUE(event_ptrs.empty());
}
