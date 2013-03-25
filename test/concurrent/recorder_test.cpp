#include "gtest/gtest.h"

#include <list>

#include "concurrent/recorder.h"
#include "concurrent/encoder.h"

using namespace se;

// Internal helper
template<typename T>
static size_t size(const std::forward_list<T>& list) {
  return std::list<T>(list.cbegin(), list.cend()).size();
}

TEST(RecorderTest, InitialBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_NE(nullptr, recorder.current_block_ptr()->outer_block_ptr());

  const std::shared_ptr<Block> most_outer_block_ptr(recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(recorder.most_outer_block_ptr(), most_outer_block_ptr);
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(1, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());
}

TEST(RecorderTest, ThenBlockWithEmptyBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> initial_block_ptr(recorder.current_block_ptr());
  recorder.begin_then(std::move(condition_ptr));

  // reuse initial block because it's empty and unconditional
  EXPECT_EQ(initial_block_ptr, recorder.current_block_ptr());

  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(*recorder.current_block_ref().condition_ptr());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_NE(nullptr, recorder.current_block_ptr()->outer_block_ptr());

  const std::shared_ptr<Block> most_outer_block_ptr(recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(recorder.most_outer_block_ptr(), most_outer_block_ptr);
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(1, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());

  recorder.end_branch();

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_EQ(2, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());
}

TEST(RecorderTest, ThenBlockWithNonemptyBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  recorder.insert_event_ptr(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, addr)));

  const std::shared_ptr<Block> most_outer_block_ptr(recorder.current_block_ptr()->outer_block_ptr());
  const std::shared_ptr<Block> initial_block_ptr(recorder.current_block_ptr());
  EXPECT_NE(initial_block_ptr, most_outer_block_ptr);
  EXPECT_EQ(recorder.most_outer_block_ptr(), most_outer_block_ptr);

  recorder.begin_then(std::move(condition_ptr));

  // cannot reuse initial, unconditional block because it's nonempty
  EXPECT_NE(initial_block_ptr, recorder.current_block_ptr());

  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(*recorder.current_block_ref().condition_ptr());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());

  recorder.end_branch();

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(3, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());
}

TEST(RecorderTest, NestedThenWithEmptyBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  recorder.insert_event_ptr(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, addr)));

  const std::shared_ptr<Block> most_outer_block_ptr(recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(recorder.most_outer_block_ptr(), most_outer_block_ptr);

  recorder.begin_then(std::move(condition_ptr));
  
  const std::shared_ptr<Block> outer_then_block_ptr(recorder.current_block_ptr());
  recorder.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, recorder.current_block_ref().outer_block_ptr());

  recorder.end_branch();

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(outer_then_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(2, size(outer_then_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), outer_then_block_ptr->end_inner_block_ptr());

  recorder.end_branch();

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  // delete unused unconditional and empty block
  EXPECT_EQ(1, size(outer_then_block_ptr->inner_block_ptrs()));

  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(3, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());
}

TEST(RecorderTest, NestedThenWithNonemptyBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  recorder.insert_event_ptr(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, addr)));

  const std::shared_ptr<Block> most_outer_block_ptr(recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(recorder.most_outer_block_ptr(), most_outer_block_ptr);

  recorder.begin_then(std::move(condition_ptr));
  
  recorder.insert_event_ptr(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, addr)));

  const std::shared_ptr<Block> outer_then_block_ptr(recorder.current_block_ptr());
  recorder.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, recorder.current_block_ref().outer_block_ptr());

  recorder.end_branch();

  recorder.insert_event_ptr(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, addr)));

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_FALSE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(outer_then_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(2, size(outer_then_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), outer_then_block_ptr->end_inner_block_ptr());

  recorder.end_branch();

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  // cannot delete unconditional and nonempty block
  EXPECT_EQ(2, size(outer_then_block_ptr->inner_block_ptrs()));

  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(3, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());
}

TEST(RecorderTest, ElseBlockWithEmptyBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> most_outer_block_ptr(recorder.current_block_ptr()->outer_block_ptr());
  const std::shared_ptr<Block> initial_block_ptr(recorder.current_block_ptr());
  EXPECT_EQ(recorder.most_outer_block_ptr(), most_outer_block_ptr);

  recorder.begin_then(std::move(condition_ptr));

  const std::shared_ptr<Block> then_block_ptr(recorder.current_block_ptr());
  // See ThenBlockWithEmptyBlock

  recorder.begin_else();

  EXPECT_NE(initial_block_ptr, recorder.current_block_ptr());
  EXPECT_NE(then_block_ptr, recorder.current_block_ptr());

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*recorder.current_block_ref().condition_ptr());
  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(not_condition.operand_ref());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_NE(nullptr, recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(1, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr()->else_block_ptr());

  recorder.end_branch();

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_EQ(2, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());
}

TEST(RecorderTest, ElseBlockWithNonemptyBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  recorder.insert_event_ptr(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, addr)));

  const std::shared_ptr<Block> most_outer_block_ptr(recorder.current_block_ptr()->outer_block_ptr());
  const std::shared_ptr<Block> initial_block_ptr(recorder.current_block_ptr());
  EXPECT_NE(initial_block_ptr, most_outer_block_ptr);
  EXPECT_EQ(recorder.most_outer_block_ptr(), most_outer_block_ptr);

  recorder.begin_then(std::move(condition_ptr));

  // See also ThenBlockWithNonemptyBlock

  const std::shared_ptr<Block> then_block_ptr(recorder.current_block_ptr());
  EXPECT_EQ(then_block_ptr, most_outer_block_ptr->end_inner_block_ptr());

  recorder.begin_else();

  EXPECT_NE(initial_block_ptr, recorder.current_block_ptr());
  EXPECT_NE(then_block_ptr, recorder.current_block_ptr());

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*recorder.current_block_ref().condition_ptr());
  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(not_condition.operand_ref());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr()->else_block_ptr());

  recorder.end_branch();

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(3, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());
}

TEST(RecorderTest, ElseWithNestedEmptyBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> most_outer_block_ptr(recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(recorder.most_outer_block_ptr(), most_outer_block_ptr);

  recorder.begin_then(std::move(condition_ptr));
  
  const std::shared_ptr<Block> outer_then_block_ptr(recorder.current_block_ptr());
  recorder.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, recorder.current_block_ref().outer_block_ptr());

  recorder.end_branch();

  // See also NestedThenWithEmptyBlock
  EXPECT_EQ(2, size(outer_then_block_ptr->inner_block_ptrs()));

  recorder.begin_else();

  // delete unused unconditional and empty block
  EXPECT_EQ(1, size(outer_then_block_ptr->inner_block_ptrs()));

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*recorder.current_block_ref().condition_ptr());
  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(not_condition.operand_ref());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(1, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr()->else_block_ptr());

  recorder.end_branch();
  EXPECT_EQ(2, size(most_outer_block_ptr->inner_block_ptrs()));

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());
}

TEST(RecorderTest, ElseWithNestedNonemptyBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> most_outer_block_ptr(recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(recorder.most_outer_block_ptr(), most_outer_block_ptr);

  recorder.begin_then(std::move(condition_ptr));
  
  const std::shared_ptr<Block> outer_then_block_ptr(recorder.current_block_ptr());
  recorder.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, recorder.current_block_ref().outer_block_ptr());

  recorder.end_branch();

  EXPECT_EQ(2, size(outer_then_block_ptr->inner_block_ptrs()));
  recorder.insert_event_ptr(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, addr)));

  recorder.begin_else();

  // cannot delete unconditional but nonempty block
  EXPECT_EQ(2, size(outer_then_block_ptr->inner_block_ptrs()));

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*recorder.current_block_ref().condition_ptr());
  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(not_condition.operand_ref());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(1, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr()->else_block_ptr());

  recorder.end_branch();

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());
}

TEST(RecorderTest, NestedElseWithEmptyBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> most_outer_block_ptr(recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(recorder.most_outer_block_ptr(), most_outer_block_ptr);

  recorder.begin_then(std::move(condition_ptr));
  
  const std::shared_ptr<Block> outer_then_block_ptr(recorder.current_block_ptr());
  recorder.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, recorder.current_block_ref().outer_block_ptr());

  recorder.begin_else();

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*recorder.current_block_ref().condition_ptr());
  const LiteralReadInstr<bool>& condition = dynamic_cast<const LiteralReadInstr<bool>&>(not_condition.operand_ref());
  EXPECT_TRUE(condition.literal());

  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(outer_then_block_ptr, recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(most_outer_block_ptr, outer_then_block_ptr->outer_block_ptr());
  EXPECT_EQ(1, size(outer_then_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), outer_then_block_ptr->end_inner_block_ptr()->else_block_ptr());

  recorder.end_branch();

  EXPECT_EQ(2, size(outer_then_block_ptr->inner_block_ptrs()));

  recorder.end_branch();

  // delete unused unconditional and empty block
  EXPECT_EQ(1, size(outer_then_block_ptr->inner_block_ptrs()));

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());
}

TEST(RecorderTest, NestedElseWithNonemptyBlock) {
  const unsigned thread_id = 3;
  Recorder recorder(thread_id);

  const MemoryAddr addr = MemoryAddr::alloc<long>();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, addr));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> most_outer_block_ptr(recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(recorder.most_outer_block_ptr(), most_outer_block_ptr);

  recorder.begin_then(std::move(condition_ptr));
  
  const std::shared_ptr<Block> outer_then_block_ptr(recorder.current_block_ptr());
  recorder.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, recorder.current_block_ref().outer_block_ptr());

  recorder.begin_else();

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*recorder.current_block_ref().condition_ptr());
  const LiteralReadInstr<bool>& condition = dynamic_cast<const LiteralReadInstr<bool>&>(not_condition.operand_ref());
  EXPECT_TRUE(condition.literal());

  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(outer_then_block_ptr, recorder.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(most_outer_block_ptr, outer_then_block_ptr->outer_block_ptr());
  EXPECT_EQ(1, size(outer_then_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), outer_then_block_ptr->end_inner_block_ptr()->else_block_ptr());

  recorder.end_branch();

  recorder.insert_event_ptr(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, addr)));

  EXPECT_EQ(2, size(outer_then_block_ptr->inner_block_ptrs()));

  recorder.end_branch();

  // cannot delete unconditional but nonempty block
  EXPECT_EQ(2, size(outer_then_block_ptr->inner_block_ptrs()));

  EXPECT_EQ(nullptr, recorder.current_block_ref().condition_ptr());
  EXPECT_TRUE(recorder.current_block_ref().body().empty());
  EXPECT_TRUE(recorder.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, recorder.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, size(most_outer_block_ptr->inner_block_ptrs()));
  EXPECT_EQ(recorder.current_block_ptr(), most_outer_block_ptr->end_inner_block_ptr());
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
