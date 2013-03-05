#include "gtest/gtest.h"

#include "concurrent.h"
#include "concurrent/memory.h"

using namespace se;

TEST(ConcurrencyTest, AllocLiteralReadInstrWithConstant) {
  std::unique_ptr<ReadInstr<long>> read_instr_ptr(alloc_read_instr(42L));

  const LiteralReadInstr<long>& literal_read_instr =
    dynamic_cast<const LiteralReadInstr<long>&>(*read_instr_ptr);
  EXPECT_EQ(42L, literal_read_instr.literal());
}

TEST(ConcurrencyTest, AllocLiteralReadInstrWithVariable) {
  long literal = 42;
  std::unique_ptr<ReadInstr<long>> read_instr_ptr(alloc_read_instr(literal));

  const LiteralReadInstr<long>& literal_read_instr =
    dynamic_cast<const LiteralReadInstr<long>&>(*read_instr_ptr);
  EXPECT_EQ(literal, literal_read_instr.literal());
}

TEST(ConcurrencyTest, AllocLiteralReadInstrWithCondition) {
  Event::reset_id(7);

  const uintptr_t ptr = 0x03fa;
  const MemoryAddr addr(ptr);
  const ConcurrentVar<char> var(addr);

  path_condition().push(3L < var);

  std::unique_ptr<ReadInstr<long long>> read_instr_ptr(alloc_read_instr(42LL));

  const LiteralReadInstr<long long>& literal_read_instr =
    dynamic_cast<const LiteralReadInstr<long long>&>(*read_instr_ptr);
  EXPECT_EQ(42LL, literal_read_instr.literal());

  const BinaryReadInstr<LSS, long, char>& lss_instr =
    dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(*read_instr_ptr->condition_ptr());

  const LiteralReadInstr<long>& left_child = dynamic_cast<const LiteralReadInstr<long>&>(lss_instr.loperand_ref());
  const BasicReadInstr<char>& right_child = dynamic_cast<const BasicReadInstr<char>&>(lss_instr.roperand_ref());
  EXPECT_EQ(3L, left_child.literal());

  EXPECT_EQ(8, right_child.event_ptr()->id());
  EXPECT_EQ(1, right_child.event_ptr()->addr().ptrs().size());
}

TEST(ConcurrencyTest, UnaryOperatorNOT) {
  uintptr_t condition_ptr = 0x03fa;

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(condition_ptr));
  const std::shared_ptr<ReadInstr<bool>> condition(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  uintptr_t ptr = 0x03fa;
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(ptr, condition));
  std::unique_ptr<ReadInstr<int>> basic_read_instr(new BasicReadInstr<int>(std::move(event_ptr)));
  std::unique_ptr<ReadInstr<bool>> instr_ptr(! std::move(basic_read_instr));
}

TEST(ConcurrencyTest, BinaryOperatorADD) {
  uintptr_t condition_ptr = 0x03fa;

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(condition_ptr));
  const std::shared_ptr<ReadInstr<bool>> condition(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  uintptr_t ptr = 0x03fa;
  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(ptr, condition));
  std::unique_ptr<ReadEvent<long>> event_ptr_b(new ReadEvent<long>(ptr + 1, condition));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));
  std::unique_ptr<ReadInstr<long>> instr_ptr(std::move(basic_read_instr_a) /* explicit move */ +
    std::unique_ptr<ReadInstr<long>>(new BasicReadInstr<long>(std::move(event_ptr_b))) /* implicit move */);
}

TEST(ConcurrencyTest, AllocConcurrentVar) {
  Event::reset_id(42);
  uintptr_t ptr = 0x03fa;
  const MemoryAddr addr(ptr);
  const ConcurrentVar<int> var(addr);

  std::unique_ptr<ReadInstr<int>> read_instr_ptr(alloc_read_instr(var));

  const BasicReadInstr<int>& basic_read_instr =
    dynamic_cast<const BasicReadInstr<int>&>(*read_instr_ptr);
  EXPECT_EQ(43, basic_read_instr.event_ptr()->id());
}

TEST(ConcurrencyTest, BinaryOperatorLSSAllocAllocWithoutCondition) {
  Event::reset_id(14);

  const uintptr_t ptr = 0x03fa;
  const MemoryAddr addr(ptr);

  ConcurrentVar<int> var(addr);

  std::unique_ptr<ReadInstr<bool>> lss_instr_ptr(var < var);

  const BinaryReadInstr<LSS, int, int>& lss_instr =
    dynamic_cast<const BinaryReadInstr<LSS, int, int>&>(*lss_instr_ptr);

  const BasicReadInstr<int>& left_child = dynamic_cast<const BasicReadInstr<int>&>(lss_instr.loperand_ref());
  const BasicReadInstr<int>& right_child = dynamic_cast<const BasicReadInstr<int>&>(lss_instr.roperand_ref());
  EXPECT_EQ(1, left_child.event_ptr()->addr().ptrs().size());
  EXPECT_EQ(1, right_child.event_ptr()->addr().ptrs().size());

  // Make no assumption about the order in which operands are evaluated
  const size_t left_id = left_child.event_ptr()->id();
  const size_t right_id = right_child.event_ptr()->id();
  EXPECT_TRUE(15 == left_id || 16 == left_id);
  EXPECT_TRUE(15 == right_id || 16 == right_id);
  EXPECT_NE(left_id, right_id);
}

TEST(ConcurrencyTest, BinaryOperatorLSSAllocLiteralWithoutCondition) {
  Event::reset_id(14);

  const uintptr_t ptr = 0x03fa;
  const MemoryAddr addr(ptr);

  ConcurrentVar<int> var(addr);

  std::unique_ptr<ReadInstr<bool>> lss_instr_ptr(var < 3);

  const BinaryReadInstr<LSS, int, int>& lss_instr =
    dynamic_cast<const BinaryReadInstr<LSS, int, int>&>(*lss_instr_ptr);

  const BasicReadInstr<int>& left_child = dynamic_cast<const BasicReadInstr<int>&>(lss_instr.loperand_ref());
  const LiteralReadInstr<int>& right_child = dynamic_cast<const LiteralReadInstr<int>&>(lss_instr.roperand_ref());
  EXPECT_EQ(1, left_child.event_ptr()->addr().ptrs().size());

  // Make no assumption about the order in which operands are evaluated
  const size_t left_id = left_child.event_ptr()->id();
  EXPECT_EQ(15, left_id);

  EXPECT_EQ(3, right_child.literal());
}

TEST(ConcurrencyTest, BinaryOperatorLSSLiteralAllocWithoutCondition) {
  Event::reset_id(14);

  const uintptr_t ptr = 0x03fa;
  const MemoryAddr addr(ptr);

  ConcurrentVar<int> var(addr);

  std::unique_ptr<ReadInstr<bool>> lss_instr_ptr(3 < var);

  const BinaryReadInstr<LSS, int, int>& lss_instr =
    dynamic_cast<const BinaryReadInstr<LSS, int, int>&>(*lss_instr_ptr);

  const LiteralReadInstr<int>& left_child = dynamic_cast<const LiteralReadInstr<int>&>(lss_instr.loperand_ref());
  const BasicReadInstr<int>& right_child = dynamic_cast<const BasicReadInstr<int>&>(lss_instr.roperand_ref());
  EXPECT_EQ(1, right_child.event_ptr()->addr().ptrs().size());

  EXPECT_EQ(3, left_child.literal());

  // Make no assumption about the order in which operands are evaluated
  const size_t right_id = right_child.event_ptr()->id();
  EXPECT_EQ(15, right_id);
}

TEST(ConcurrencyTest, ConcurrentVarAssignmentWithoutCondition) {
  Event::reset_id(7);

  ConcurrentVar<char> char_integer;
  ConcurrentVar<long> long_integer;

  long_integer = 3L + char_integer;

  const BinaryReadInstr<ADD, long, char>& add_instr =
    dynamic_cast<const BinaryReadInstr<ADD, long, char>&>(long_integer.instr_ref());
  const LiteralReadInstr<long>& loperand = dynamic_cast<const LiteralReadInstr<long>&>(add_instr.loperand_ref());
  const BasicReadInstr<char>& roperand = dynamic_cast<const BasicReadInstr<char>&>(add_instr.roperand_ref());

  EXPECT_EQ(1, roperand.event_ptr()->addr().ptrs().size());
  EXPECT_EQ(3L, loperand.literal());
  EXPECT_EQ(9, roperand.event_ptr()->id());
}
