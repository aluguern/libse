#include "concurrent/slice.h"
#include "gtest/gtest.h"

using namespace se;

#define READ_EVENT_ID(id) (id)
#define WRITE_EVENT_ID(id) (id)

TEST(SliceTest, LoopPolicy) {
  constexpr LoopPolicy p(make_loop_policy<7, 2>());
  static_assert(7 == p.id(), "Wrong loop ID");
  static_assert(2 == p.unwinding_bound(), "Wrong loop unwinding bound");
}

TEST(SliceTest, CopyLoopPolicy) {
  constexpr LoopPolicy p(make_loop_policy<7, 2>());
  constexpr LoopPolicy q(p);

  static_assert(7 == q.id(), "Wrong loop ID");
  static_assert(2 == q.unwinding_bound(), "Wrong loop unwinding bound");
}

TEST(SliceTest, ConstLoop) {
  constexpr LoopPolicy p(make_loop_policy<7, 1>());
  constexpr Loop const_loop(p);

  static_assert(7 == const_loop.policy_id(), "Wrong loop ID");
  static_assert(1 == const_loop.unwinding_bound(), "Wrong loop unwinding bound");
  EXPECT_EQ(1, const_loop.unwinding_counter());
}

TEST(SliceTest, MoveLoop) {
  constexpr LoopPolicy p(make_loop_policy<7, 1>());
  constexpr Loop const_loop = Loop(p);

  static_assert(7 == const_loop.policy_id(), "Wrong loop ID");
  static_assert(1 == const_loop.unwinding_bound(), "Wrong loop unwinding bound");
  EXPECT_EQ(1, const_loop.unwinding_counter());
}

TEST(SliceTest, DecrementLoopUnwindingCounter) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";

  constexpr LoopPolicy p(make_loop_policy<7, 1>());
  Loop loop(p);

  EXPECT_EQ(7, loop.policy_id());
  EXPECT_EQ(1, loop.unwinding_bound());
  EXPECT_EQ(1, loop.unwinding_counter());

  loop.decrement_unwinding_counter();
  EXPECT_EQ(0, loop.unwinding_counter());

  EXPECT_EXIT(loop.decrement_unwinding_counter(), ::testing::KilledBySignal(SIGABRT), "Assertion");
}

TEST(SliceTest, InitialBlock) {
  Slice slice;

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_NE(nullptr, slice.current_block_ptr()->outer_block_ptr());

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(slice.most_outer_block_ptr(), most_outer_block_ptr);
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(1, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());
}

TEST(SliceTest, ThenBlockWithEmptyBlock) {
  Slice slice;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new LiteralReadInstr<long>(42L));
  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> initial_block_ptr(slice.current_block_ptr());
  slice.begin_then(std::move(condition_ptr));

  // reuse initial block because it's empty and unconditional
  EXPECT_EQ(initial_block_ptr, slice.current_block_ptr());

  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(*slice.current_block_ref().condition_ptr());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_NE(nullptr, slice.current_block_ptr()->outer_block_ptr());

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(slice.most_outer_block_ptr(), most_outer_block_ptr);
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(1, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());

  slice.end_branch();

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_EQ(2, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());
}

TEST(SliceTest, ThenBlockWithNonemptyBlock) {
  const unsigned thread_id = 3;
  Slice slice;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, zone));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  slice.append(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, zone)));

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  const std::shared_ptr<Block> initial_block_ptr(slice.current_block_ptr());
  EXPECT_NE(initial_block_ptr, most_outer_block_ptr);
  EXPECT_EQ(slice.most_outer_block_ptr(), most_outer_block_ptr);

  slice.begin_then(std::move(condition_ptr));

  // cannot reuse initial, unconditional block because it's nonempty
  EXPECT_NE(initial_block_ptr, slice.current_block_ptr());

  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(*slice.current_block_ref().condition_ptr());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());

  slice.end_branch();

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(3, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());
}

TEST(SliceTest, NestedThenWithEmptyBlock) {
  const unsigned thread_id = 3;
  Slice slice;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, zone));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  slice.append(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, zone)));

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(slice.most_outer_block_ptr(), most_outer_block_ptr);

  slice.begin_then(std::move(condition_ptr));
  
  const std::shared_ptr<Block> outer_then_block_ptr(slice.current_block_ptr());
  slice.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, slice.current_block_ref().outer_block_ptr());

  slice.end_branch();

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(outer_then_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(2, outer_then_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), outer_then_block_ptr->inner_block_ptrs().back());

  slice.end_branch();

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  // delete unused unconditional and empty block
  EXPECT_EQ(1, outer_then_block_ptr->inner_block_ptrs().size());

  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(3, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());
}

TEST(SliceTest, NestedThenWithNonemptyBlock) {
  const unsigned thread_id = 3;
  Slice slice;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, zone));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  slice.append(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, zone)));

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(slice.most_outer_block_ptr(), most_outer_block_ptr);

  slice.begin_then(std::move(condition_ptr));
  
  slice.append(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, zone)));

  const std::shared_ptr<Block> outer_then_block_ptr(slice.current_block_ptr());
  slice.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, slice.current_block_ref().outer_block_ptr());

  slice.end_branch();

  slice.append(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, zone)));

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_FALSE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(outer_then_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(2, outer_then_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), outer_then_block_ptr->inner_block_ptrs().back());

  slice.end_branch();

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  // cannot delete unconditional and nonempty block
  EXPECT_EQ(2, outer_then_block_ptr->inner_block_ptrs().size());

  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(3, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());
}

TEST(SliceTest, ElseBlockWithEmptyBlock) {
  Slice slice;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new LiteralReadInstr<long>(42L));
  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  const std::shared_ptr<Block> initial_block_ptr(slice.current_block_ptr());
  EXPECT_EQ(slice.most_outer_block_ptr(), most_outer_block_ptr);

  slice.begin_then(std::move(condition_ptr));

  const std::shared_ptr<Block> then_block_ptr(slice.current_block_ptr());
  // See ThenBlockWithEmptyBlock

  slice.begin_else();

  EXPECT_NE(initial_block_ptr, slice.current_block_ptr());
  EXPECT_NE(then_block_ptr, slice.current_block_ptr());

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*slice.current_block_ref().condition_ptr());
  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(not_condition.operand_ref());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_NE(nullptr, slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(1, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back()->else_block_ptr());

  slice.end_branch();

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_EQ(2, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());
}

TEST(SliceTest, ElseBlockWithNonemptyBlock) {
  const unsigned thread_id = 3;
  Slice slice;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, zone));
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new BasicReadInstr<long>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  slice.append(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, zone)));

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  const std::shared_ptr<Block> initial_block_ptr(slice.current_block_ptr());
  EXPECT_NE(initial_block_ptr, most_outer_block_ptr);
  EXPECT_EQ(slice.most_outer_block_ptr(), most_outer_block_ptr);

  slice.begin_then(std::move(condition_ptr));

  // See also ThenBlockWithNonemptyBlock

  const std::shared_ptr<Block> then_block_ptr(slice.current_block_ptr());
  EXPECT_EQ(then_block_ptr, most_outer_block_ptr->inner_block_ptrs().back());

  slice.begin_else();

  EXPECT_NE(initial_block_ptr, slice.current_block_ptr());
  EXPECT_NE(then_block_ptr, slice.current_block_ptr());

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*slice.current_block_ref().condition_ptr());
  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(not_condition.operand_ref());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back()->else_block_ptr());

  slice.end_branch();

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());
  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(3, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());
}

TEST(SliceTest, ElseWithNestedEmptyBlock) {
  Slice slice;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new LiteralReadInstr<long>(42L));
  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(slice.most_outer_block_ptr(), most_outer_block_ptr);

  slice.begin_then(std::move(condition_ptr));
  
  const std::shared_ptr<Block> outer_then_block_ptr(slice.current_block_ptr());
  slice.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, slice.current_block_ref().outer_block_ptr());

  slice.end_branch();

  // See also NestedThenWithEmptyBlock
  EXPECT_EQ(2, outer_then_block_ptr->inner_block_ptrs().size());

  slice.begin_else();

  // delete unused unconditional and empty block
  EXPECT_EQ(1, outer_then_block_ptr->inner_block_ptrs().size());

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*slice.current_block_ref().condition_ptr());
  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(not_condition.operand_ref());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(1, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back()->else_block_ptr());

  slice.end_branch();
  EXPECT_EQ(2, most_outer_block_ptr->inner_block_ptrs().size());

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());
}

TEST(SliceTest, ElseWithNestedNonemptyBlock) {
  const unsigned thread_id = 3;
  Slice slice;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new LiteralReadInstr<long>(42L));
  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(slice.most_outer_block_ptr(), most_outer_block_ptr);

  slice.begin_then(std::move(condition_ptr));
  
  const std::shared_ptr<Block> outer_then_block_ptr(slice.current_block_ptr());
  slice.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, slice.current_block_ref().outer_block_ptr());

  slice.end_branch();

  EXPECT_EQ(2, outer_then_block_ptr->inner_block_ptrs().size());
  slice.append(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, zone)));

  slice.begin_else();

  // cannot delete unconditional but nonempty block
  EXPECT_EQ(2, outer_then_block_ptr->inner_block_ptrs().size());

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*slice.current_block_ref().condition_ptr());
  const BinaryReadInstr<LSS, long, char>& condition = dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(not_condition.operand_ref());
  const LiteralReadInstr<char>& roperand = dynamic_cast<const LiteralReadInstr<char>&>(condition.roperand_ref());
  EXPECT_EQ('Z', roperand.literal());

  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(1, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back()->else_block_ptr());

  slice.end_branch();

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());
}

TEST(SliceTest, NestedElseWithEmptyBlock) {
  Slice slice;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new LiteralReadInstr<long>(42L));
  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(slice.most_outer_block_ptr(), most_outer_block_ptr);

  slice.begin_then(std::move(condition_ptr));
  
  const std::shared_ptr<Block> outer_then_block_ptr(slice.current_block_ptr());
  slice.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, slice.current_block_ref().outer_block_ptr());

  slice.begin_else();

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*slice.current_block_ref().condition_ptr());
  const LiteralReadInstr<bool>& condition = dynamic_cast<const LiteralReadInstr<bool>&>(not_condition.operand_ref());
  EXPECT_TRUE(condition.literal());

  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(outer_then_block_ptr, slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(most_outer_block_ptr, outer_then_block_ptr->outer_block_ptr());
  EXPECT_EQ(1, outer_then_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), outer_then_block_ptr->inner_block_ptrs().back()->else_block_ptr());

  slice.end_branch();

  EXPECT_EQ(2, outer_then_block_ptr->inner_block_ptrs().size());

  slice.end_branch();

  // delete unused unconditional and empty block
  EXPECT_EQ(1, outer_then_block_ptr->inner_block_ptrs().size());

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());
}

TEST(SliceTest, NestedElseWithNonemptyBlock) {
  const unsigned thread_id = 3;
  Slice slice;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadInstr<long>> linstr_ptr(new LiteralReadInstr<long>(42L));
  std::unique_ptr<ReadInstr<char>> rinstr_ptr(new LiteralReadInstr<char>('Z'));

  std::unique_ptr<ReadInstr<bool>> condition_ptr(new BinaryReadInstr<LSS, long, char>(
    std::move(linstr_ptr), std::move(rinstr_ptr)));

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(slice.most_outer_block_ptr(), most_outer_block_ptr);

  slice.begin_then(std::move(condition_ptr));
  
  const std::shared_ptr<Block> outer_then_block_ptr(slice.current_block_ptr());
  slice.begin_then(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)));

  EXPECT_EQ(outer_then_block_ptr, slice.current_block_ref().outer_block_ptr());

  slice.begin_else();

  const UnaryReadInstr<NOT, bool>& not_condition = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*slice.current_block_ref().condition_ptr());
  const LiteralReadInstr<bool>& condition = dynamic_cast<const LiteralReadInstr<bool>&>(not_condition.operand_ref());
  EXPECT_TRUE(condition.literal());

  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(outer_then_block_ptr, slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(most_outer_block_ptr, outer_then_block_ptr->outer_block_ptr());
  EXPECT_EQ(1, outer_then_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), outer_then_block_ptr->inner_block_ptrs().back()->else_block_ptr());

  slice.end_branch();

  slice.append(std::unique_ptr<Event>(new ReadEvent<bool>(thread_id, zone)));

  EXPECT_EQ(2, outer_then_block_ptr->inner_block_ptrs().size());

  slice.end_branch();

  // cannot delete unconditional but nonempty block
  EXPECT_EQ(2, outer_then_block_ptr->inner_block_ptrs().size());

  EXPECT_EQ(nullptr, slice.current_block_ref().condition_ptr());
  EXPECT_TRUE(slice.current_block_ref().body().empty());
  EXPECT_TRUE(slice.current_block_ref().inner_block_ptrs().empty());

  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ref().outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->outer_block_ptr());
  EXPECT_EQ(nullptr, most_outer_block_ptr->condition_ptr());
  EXPECT_TRUE(most_outer_block_ptr->body().empty());
  EXPECT_EQ(2, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(slice.current_block_ptr(), most_outer_block_ptr->inner_block_ptrs().back());
}

TEST(SliceTest, SimpleLoopWithReuse) {
  Slice slice;

  constexpr LoopPolicy policy(make_loop_policy<7, 2>());
  bool continue_unwinding = true;

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(most_outer_block_ptr, slice.most_outer_block_ptr());

  const std::shared_ptr<Block> initial_block_ptr(slice.current_block_ptr());
  EXPECT_TRUE(initial_block_ptr->body().empty());
  EXPECT_TRUE(initial_block_ptr->inner_block_ptrs().empty());
  EXPECT_EQ(nullptr, initial_block_ptr->condition_ptr());

  // k = 1
  continue_unwinding = slice.unwind_loop(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)), policy);
  EXPECT_TRUE(continue_unwinding);

  // reuse empty and conditional initial block
  EXPECT_EQ(initial_block_ptr, slice.current_block_ptr());
  EXPECT_TRUE(initial_block_ptr->body().empty());
  EXPECT_TRUE(initial_block_ptr->inner_block_ptrs().empty());
  EXPECT_NE(nullptr, initial_block_ptr->condition_ptr());

  // k = 2
  continue_unwinding = slice.unwind_loop(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)), policy);
  EXPECT_TRUE(continue_unwinding);

  const std::shared_ptr<Block> second_unwound_loop_block_ptr(slice.current_block_ptr());
  EXPECT_NE(initial_block_ptr, second_unwound_loop_block_ptr);
  EXPECT_EQ(initial_block_ptr, second_unwound_loop_block_ptr->outer_block_ptr());
  EXPECT_TRUE(second_unwound_loop_block_ptr->body().empty());
  EXPECT_TRUE(second_unwound_loop_block_ptr->inner_block_ptrs().empty());
  EXPECT_NE(nullptr, second_unwound_loop_block_ptr->condition_ptr());

  // k = 3, stop unrolling!
  continue_unwinding = slice.unwind_loop(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)), policy);
  EXPECT_FALSE(continue_unwinding);

  EXPECT_NE(second_unwound_loop_block_ptr, slice.current_block_ptr());
  EXPECT_NE(initial_block_ptr, slice.current_block_ptr());
  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(2, most_outer_block_ptr->inner_block_ptrs().size());
}

TEST(SliceTest, SimpleLoopWithoutReuse) {
  const unsigned thread_id = 3;
  Slice slice;

  constexpr LoopPolicy policy(make_loop_policy<7, 2>());
  bool continue_unwinding = true;

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(1, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(most_outer_block_ptr, slice.most_outer_block_ptr());

  slice.append(std::unique_ptr<Event>(new ReadEvent<char>(thread_id,  Zone::unique_atom())));

  const std::shared_ptr<Block> initial_block_ptr(slice.current_block_ptr());
  EXPECT_FALSE(initial_block_ptr->body().empty());
  EXPECT_TRUE(initial_block_ptr->inner_block_ptrs().empty());
  EXPECT_EQ(nullptr, initial_block_ptr->condition_ptr());

  // k = 1
  continue_unwinding = slice.unwind_loop(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)), policy);
  EXPECT_TRUE(continue_unwinding);

  // cannot reuse nonempty initial block
  const std::shared_ptr<Block> first_unwound_loop_block_ptr(slice.current_block_ptr());
  EXPECT_NE(initial_block_ptr, first_unwound_loop_block_ptr);
  EXPECT_TRUE(first_unwound_loop_block_ptr->body().empty());
  EXPECT_TRUE(first_unwound_loop_block_ptr->inner_block_ptrs().empty());
  EXPECT_NE(nullptr, first_unwound_loop_block_ptr->condition_ptr());
  EXPECT_TRUE(initial_block_ptr->inner_block_ptrs().empty());
  EXPECT_FALSE(initial_block_ptr->body().empty());
  EXPECT_EQ(nullptr, initial_block_ptr->condition_ptr());

  // k = 2
  continue_unwinding = slice.unwind_loop(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)), policy);
  EXPECT_TRUE(continue_unwinding);

  const std::shared_ptr<Block> second_unwound_loop_block_ptr(slice.current_block_ptr());
  EXPECT_NE(initial_block_ptr, second_unwound_loop_block_ptr);
  EXPECT_EQ(first_unwound_loop_block_ptr, second_unwound_loop_block_ptr->outer_block_ptr());
  EXPECT_TRUE(second_unwound_loop_block_ptr->body().empty());
  EXPECT_TRUE(second_unwound_loop_block_ptr->inner_block_ptrs().empty());
  EXPECT_NE(nullptr, second_unwound_loop_block_ptr->condition_ptr());

  // k = 3, stop unrolling!
  continue_unwinding = slice.unwind_loop(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)), policy);
  EXPECT_FALSE(continue_unwinding);

  EXPECT_NE(second_unwound_loop_block_ptr, slice.current_block_ptr());
  EXPECT_NE(first_unwound_loop_block_ptr, slice.current_block_ptr());
  EXPECT_NE(initial_block_ptr, slice.current_block_ptr());
  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(3, most_outer_block_ptr->inner_block_ptrs().size());
}

TEST(SliceTest, NestedLoop) {
  const unsigned thread_id = 3;
  Slice slice;

  constexpr LoopPolicy outer_policy(make_loop_policy<7, 1>());
  constexpr LoopPolicy inner_policy(make_loop_policy<8, 1>());
  bool continue_unwinding = true;

  slice.append(std::unique_ptr<Event>(new ReadEvent<char>(thread_id,  Zone::unique_atom())));

  const std::shared_ptr<Block> most_outer_block_ptr(slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(1, most_outer_block_ptr->inner_block_ptrs().size());
  EXPECT_EQ(most_outer_block_ptr, slice.most_outer_block_ptr());

  const std::shared_ptr<Block> initial_block_ptr(slice.current_block_ptr());
  EXPECT_FALSE(initial_block_ptr->body().empty());
  EXPECT_TRUE(initial_block_ptr->inner_block_ptrs().empty());
  EXPECT_EQ(nullptr, initial_block_ptr->condition_ptr());

  // k = 1
  continue_unwinding = slice.unwind_loop(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)), outer_policy);
  EXPECT_TRUE(continue_unwinding);

  // cannot reuse nonempty initial block
  const std::shared_ptr<Block> first_unwound_loop_block_ptr(slice.current_block_ptr());
  EXPECT_NE(initial_block_ptr, first_unwound_loop_block_ptr);
  EXPECT_TRUE(first_unwound_loop_block_ptr->body().empty());
  EXPECT_TRUE(first_unwound_loop_block_ptr->inner_block_ptrs().empty());
  EXPECT_NE(nullptr, first_unwound_loop_block_ptr->condition_ptr());
  EXPECT_TRUE(initial_block_ptr->inner_block_ptrs().empty());
  EXPECT_FALSE(initial_block_ptr->body().empty());
  EXPECT_EQ(nullptr, initial_block_ptr->condition_ptr());

  // j = 1
  continue_unwinding = slice.unwind_loop(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)), inner_policy);
  EXPECT_TRUE(continue_unwinding);

  const std::shared_ptr<Block> second_unwound_loop_block_ptr(slice.current_block_ptr());
  EXPECT_NE(initial_block_ptr, second_unwound_loop_block_ptr);
  EXPECT_EQ(first_unwound_loop_block_ptr, second_unwound_loop_block_ptr->outer_block_ptr());
  EXPECT_TRUE(second_unwound_loop_block_ptr->body().empty());
  EXPECT_TRUE(second_unwound_loop_block_ptr->inner_block_ptrs().empty());
  EXPECT_NE(nullptr, second_unwound_loop_block_ptr->condition_ptr());

  // j = 2, stop inner loop unrolling!
  continue_unwinding = slice.unwind_loop(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)), inner_policy);
  EXPECT_FALSE(continue_unwinding);

  EXPECT_NE(second_unwound_loop_block_ptr, slice.current_block_ptr());
  EXPECT_NE(first_unwound_loop_block_ptr, slice.current_block_ptr());
  EXPECT_NE(initial_block_ptr, slice.current_block_ptr());
  EXPECT_EQ(first_unwound_loop_block_ptr, slice.current_block_ptr()->outer_block_ptr());

  EXPECT_EQ(2, first_unwound_loop_block_ptr->inner_block_ptrs().size());

  // k = 2, stop outer loop unrolling!
  continue_unwinding = slice.unwind_loop(std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)), outer_policy);
  EXPECT_FALSE(continue_unwinding);

  EXPECT_NE(second_unwound_loop_block_ptr, slice.current_block_ptr());
  EXPECT_NE(first_unwound_loop_block_ptr, slice.current_block_ptr());
  EXPECT_NE(initial_block_ptr, slice.current_block_ptr());
  EXPECT_EQ(most_outer_block_ptr, slice.current_block_ptr()->outer_block_ptr());
  EXPECT_EQ(3, most_outer_block_ptr->inner_block_ptrs().size());
}

TEST(SliceTest, Body) {
  Event::reset_id(3);

  const unsigned thread_id = 3;

  const Zone char_zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<char>> char_event_ptr(new ReadEvent<char>(thread_id, char_zone));
  std::unique_ptr<ReadInstr<char>> char_instr_ptr(new BasicReadInstr<char>(std::move(char_event_ptr)));

  const Zone long_zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<long>> long_event_ptr(new ReadEvent<long>(thread_id, long_zone));
  std::unique_ptr<ReadInstr<long>> long_instr_ptr(new BasicReadInstr<long>(std::move(long_event_ptr)));

  std::unique_ptr<ReadInstr<long>> instr_ptr(
    new BinaryReadInstr<ADD, char, long>(std::move(char_instr_ptr), std::move(long_instr_ptr)));

  const Zone write_zone = Zone::unique_atom();

  Slice slice;
  slice.append_all<long>(*instr_ptr);

  const std::shared_ptr<DirectWriteEvent<long>> write_event_ptr(new DirectWriteEvent<long>(
    thread_id, write_zone, std::move(instr_ptr)));

  slice.append(write_event_ptr);

  std::forward_list<std::shared_ptr<Event>>& event_ptrs = slice.current_block_body();

  const ReadEvent<char>& read_char_event = dynamic_cast<const ReadEvent<char>&>(*event_ptrs.front());
  EXPECT_EQ(READ_EVENT_ID(3), read_char_event.event_id());

  event_ptrs.pop_front();

  const ReadEvent<long>& read_long_event = dynamic_cast<const ReadEvent<long>&>(*event_ptrs.front());
  EXPECT_EQ(READ_EVENT_ID(4), read_long_event.event_id());

  event_ptrs.pop_front();

  const WriteEvent<long>& write_long_event = dynamic_cast<const WriteEvent<long>&>(*event_ptrs.front());
  EXPECT_EQ(WRITE_EVENT_ID(5), write_long_event.event_id());

  event_ptrs.pop_front();
  EXPECT_TRUE(event_ptrs.empty());
}
