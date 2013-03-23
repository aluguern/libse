#include "gtest/gtest.h"

#include "concurrent/recorder.h"
#include "concurrent/encoder.h"

using namespace se;

TEST(RecorderTest, InitialRootBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);
  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_body().empty());
}

TEST(RecorderTest, ThenBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> main_block_ptr(recorder.current_block_ptr());

  recorder.begin_then_block(std::move(condition_ptr));

  EXPECT_NE(main_block_ptr, recorder.current_block_ptr());

  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(*recorder.current_block_ref().condition_ptr());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_EQ(main_block_ptr, recorder.current_block_ref().prev_block_ptr());
  EXPECT_EQ(recorder.current_block_ptr(), main_block_ptr->then_block_ptr());
  EXPECT_EQ(recorder.current_block_ptr(), recorder.current_block_ref().next_block_ptr()->prev_block_ptr());

  EXPECT_EQ(nullptr, recorder.current_block_ref().next_block_ptr()->condition_ptr());
  EXPECT_EQ(nullptr, main_block_ptr->condition_ptr());
  EXPECT_EQ(nullptr, main_block_ptr->next_block_ptr());

  const std::shared_ptr<Block> next_block_ptr(recorder.current_block_ref().next_block_ptr());

  recorder.end_block();

  EXPECT_EQ(next_block_ptr, recorder.current_block_ptr());
}

TEST(RecorderTest, ElseBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> main_block_ptr(recorder.current_block_ptr());

  recorder.begin_then_block(std::move(condition_ptr));

  EXPECT_NE(main_block_ptr, recorder.current_block_ptr());

  EXPECT_EQ(main_block_ptr, recorder.current_block_ref().prev_block_ptr());
  EXPECT_EQ(recorder.current_block_ptr(), main_block_ptr->then_block_ptr());
  EXPECT_EQ(nullptr, main_block_ptr->else_block_ptr());
  EXPECT_EQ(recorder.current_block_ptr(), recorder.current_block_ref().next_block_ptr()->prev_block_ptr());

  EXPECT_EQ(nullptr, recorder.current_block_ref().next_block_ptr()->condition_ptr());
  EXPECT_EQ(nullptr, main_block_ptr->condition_ptr());
  EXPECT_EQ(nullptr, main_block_ptr->next_block_ptr());

  const std::shared_ptr<Block> then_block_ptr(recorder.current_block_ptr());
  const std::shared_ptr<Block> next_block_ptr(then_block_ptr->next_block_ptr());

  recorder.begin_else_block();

  EXPECT_NE(main_block_ptr, recorder.current_block_ptr());

  const UnaryReadInstr<NOT, bool>& else_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*recorder.current_block_ref().condition_ptr());
  const BinaryReadInstr<LSS, long, char>& then_condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(else_condition.operand_ref());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(then_condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_EQ(main_block_ptr->else_block_ptr(), recorder.current_block_ptr());
  EXPECT_EQ(then_block_ptr->next_block_ptr(), recorder.current_block_ref().next_block_ptr());
  EXPECT_EQ(main_block_ptr, recorder.current_block_ref().prev_block_ptr());

  recorder.end_block();

  EXPECT_EQ(next_block_ptr, then_block_ptr->next_block_ptr());
  EXPECT_EQ(next_block_ptr, recorder.current_block_ptr());
}

TEST(RecorderTest, Body) {
  Event::reset_id(3);

  const unsigned thread_id = 3;

  const MemoryAddr char_addr = MemoryAddr::alloc<char>();
  std::unique_ptr<ReadEvent<char>> char_event_ptr(new ReadEvent<char>(thread_id, char_addr));
  std::unique_ptr<ReadInstr<char>> char_instr_ptr(new BasicReadInstr<char>(std::move(char_event_ptr)));

  const MemoryAddr long_addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> long_event_ptr(new ReadEvent<long>(thread_id, long_addr));
  std::unique_ptr<ReadInstr<long>> long_instr_ptr(new BasicReadInstr<long>(std::move(long_event_ptr)));

  std::unique_ptr<ReadInstr<long>> instr_ptr(
    new BinaryReadInstr<ADD, char, long>(std::move(char_instr_ptr), std::move(long_instr_ptr)));

  const MemoryAddr write_addr = MemoryAddr::alloc<long>();

  Recorder recorder(thread_id);
  recorder.instr(write_addr, std::move(instr_ptr));

  std::forward_list<std::shared_ptr<Event>>& event_ptrs = recorder.current_block_body();

  const WriteEvent<long>& write_long_event = dynamic_cast<const WriteEvent<long>&>(*event_ptrs.front());
  EXPECT_EQ(2*5+1, write_long_event.event_id());

  event_ptrs.pop_front();

  const ReadEvent<long>& read_long_event = dynamic_cast<const ReadEvent<long>&>(*event_ptrs.front());
  EXPECT_EQ(2*4, read_long_event.event_id());

  event_ptrs.pop_front();

  const ReadEvent<char>& read_char_event = dynamic_cast<const ReadEvent<char>&>(*event_ptrs.front());
  EXPECT_EQ(2*3, read_char_event.event_id());

  event_ptrs.pop_front();
  EXPECT_TRUE(event_ptrs.empty());
}

TEST(RecorderTest, SeriesParallelGraph) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  
}
