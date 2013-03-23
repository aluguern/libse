#include "gtest/gtest.h"

#include <sstream>

#include "concurrent.h"
#include "concurrent/memory.h"

using namespace se;

TEST(ConcurrencyTest, AllocLiteralReadInstrWithConstant) {
  init_recorder();

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(alloc_read_instr(42L));

  const LiteralReadInstr<long>& literal_read_instr =
    dynamic_cast<const LiteralReadInstr<long>&>(*read_instr_ptr);
  EXPECT_EQ(42L, literal_read_instr.literal());
}

TEST(ConcurrencyTest, AllocLiteralReadInstrWithVariable) {
  init_recorder();

  long literal = 42;
  std::unique_ptr<ReadInstr<long>> read_instr_ptr(alloc_read_instr(literal));

  const LiteralReadInstr<long>& literal_read_instr =
    dynamic_cast<const LiteralReadInstr<long>&>(*read_instr_ptr);
  EXPECT_EQ(literal, literal_read_instr.literal());
}

TEST(ConcurrencyTest, AllocLiteralReadInstrWithCondition) {
  init_recorder();
  Event::reset_id(7);

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<char>();
  std::unique_ptr<ReadEvent<char>> event_ptr(new ReadEvent<char>(thread_id, addr));
  std::unique_ptr<ReadInstr<char>> basic_read_instr_ptr(new BasicReadInstr<char>(std::move(event_ptr)));

  recorder_ptr()->begin_then_block(3L < std::move(basic_read_instr_ptr));

  std::unique_ptr<ReadInstr<long long>> read_instr_ptr(alloc_read_instr(42LL));

  const LiteralReadInstr<long long>& literal_read_instr =
    dynamic_cast<const LiteralReadInstr<long long>&>(*read_instr_ptr);
  EXPECT_EQ(42LL, literal_read_instr.literal());

  const BinaryReadInstr<LSS, long, char>& lss_instr =
    dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(*read_instr_ptr->condition_ptr());

  const LiteralReadInstr<long>& loperand = dynamic_cast<const LiteralReadInstr<long>&>(lss_instr.loperand_ref());
  const BasicReadInstr<char>& roperand = dynamic_cast<const BasicReadInstr<char>&>(lss_instr.roperand_ref());
  EXPECT_EQ(3L, loperand.literal());

  EXPECT_EQ(2*7, roperand.event_ptr()->event_id());
}

TEST(ConcurrencyTest, UnaryOperatorNOTWithCondition) {
  init_recorder();
  Event::reset_id(14);

  const unsigned thread_id = 3;
  const MemoryAddr condition_addr = MemoryAddr::alloc<bool>();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_addr));
  const std::shared_ptr<ReadInstr<bool>> condition(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  const MemoryAddr addr = MemoryAddr::alloc<int>();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, addr, condition));
  std::unique_ptr<ReadInstr<int>> basic_read_instr(new BasicReadInstr<int>(std::move(event_ptr)));
  std::unique_ptr<ReadInstr<bool>> instr_ptr(! std::move(basic_read_instr));

  const UnaryReadInstr<NOT, int>& instr = dynamic_cast<const UnaryReadInstr<NOT, int>&>(*instr_ptr);

  const BasicReadInstr<int>& operand = dynamic_cast<const BasicReadInstr<int>&>(instr.operand_ref());
  EXPECT_TRUE(2*15 == operand.event_ptr()->event_id());
}

TEST(ConcurrencyTest, BinaryOperatorLSSWithoutCondition) {
  init_recorder();
  Event::reset_id(14);

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int>();

  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(thread_id, addr));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_ptr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));

  std::unique_ptr<ReadEvent<int>> event_ptr_b(new ReadEvent<int>(thread_id, addr));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_ptr_b(new BasicReadInstr<int>(std::move(event_ptr_b)));

  std::unique_ptr<ReadInstr<bool>> instr_ptr(std::move(basic_read_instr_ptr_a) < std::move(basic_read_instr_ptr_b));

  const BinaryReadInstr<LSS, int, int>& instr =
    dynamic_cast<const BinaryReadInstr<LSS, int, int>&>(*instr_ptr);

  const BasicReadInstr<int>& loperand = dynamic_cast<const BasicReadInstr<int>&>(instr.loperand_ref());
  const BasicReadInstr<int>& roperand = dynamic_cast<const BasicReadInstr<int>&>(instr.roperand_ref());

  // Make no assumption about the order in which operands are evaluated
  const size_t left_id = loperand.event_ptr()->event_id();
  const size_t right_id = roperand.event_ptr()->event_id();
  EXPECT_TRUE(2*14 == left_id || 2*15 == left_id);
  EXPECT_TRUE(2*14 == right_id || 2*15 == right_id);
  EXPECT_NE(left_id, right_id);
}

TEST(ConcurrencyTest, BinaryOperatorADDWithCondition) {
  init_recorder();
  Event::reset_id(14);

  const unsigned thread_id = 3;
  const MemoryAddr condition_addr = MemoryAddr::alloc<bool>();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_addr));
  const std::shared_ptr<ReadInstr<bool>> condition(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  const MemoryAddr addr_a = MemoryAddr::alloc<int>();
  const MemoryAddr addr_b = MemoryAddr::alloc<long>();

  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(thread_id, addr_a, condition));
  std::unique_ptr<ReadEvent<long>> event_ptr_b(new ReadEvent<long>(thread_id, addr_b, condition));

  std::unique_ptr<ReadInstr<int>> basic_read_instr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));
  std::unique_ptr<ReadInstr<long>> instr_ptr(std::move(basic_read_instr_a) /* explicit move */ +
    std::unique_ptr<ReadInstr<long>>(new BasicReadInstr<long>(std::move(event_ptr_b))) /* implicit move */);

  const BinaryReadInstr<ADD, int, long>& instr = dynamic_cast<const BinaryReadInstr<ADD, int, long>&>(*instr_ptr);

  const BasicReadInstr<int>& loperand = dynamic_cast<const BasicReadInstr<int>&>(instr.loperand_ref());
  const BasicReadInstr<long>& roperand = dynamic_cast<const BasicReadInstr<long>&>(instr.roperand_ref());

  // Make no assumption about the order in which operands are evaluated
  const size_t left_id = loperand.event_ptr()->event_id();
  const size_t right_id = roperand.event_ptr()->event_id();
  EXPECT_TRUE(2*15 == left_id || 2*16 == left_id);
  EXPECT_TRUE(2*15 == right_id || 2*16 == right_id);
  EXPECT_NE(left_id, right_id);
}

TEST(ConcurrencyTest, BinaryOperatorLSSReadInstrPointerLiteralWithoutCondition) {
  init_recorder();
  Event::reset_id(14);

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, addr));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_ptr(new BasicReadInstr<int>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<bool>> lss_instr_ptr(std::move(basic_read_instr_ptr) < 3);

  const BinaryReadInstr<LSS, int, int>& lss_instr =
    dynamic_cast<const BinaryReadInstr<LSS, int, int>&>(*lss_instr_ptr);

  const BasicReadInstr<int>& loperand = dynamic_cast<const BasicReadInstr<int>&>(lss_instr.loperand_ref());
  const LiteralReadInstr<int>& roperand = dynamic_cast<const LiteralReadInstr<int>&>(lss_instr.roperand_ref());

  // Make no assumption about the order in which operands are evaluated
  const size_t left_id = loperand.event_ptr()->event_id();
  EXPECT_EQ(2*14, left_id);

  EXPECT_EQ(3, roperand.literal());
}

TEST(ConcurrencyTest, BinaryOperatorLSSLiteralReadInstrPointerWithoutCondition) {
  init_recorder();
  Event::reset_id(14);

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, addr));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_ptr(new BasicReadInstr<int>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<bool>> lss_instr_ptr(3 < std::move(basic_read_instr_ptr));

  const BinaryReadInstr<LSS, int, int>& lss_instr =
    dynamic_cast<const BinaryReadInstr<LSS, int, int>&>(*lss_instr_ptr);

  const LiteralReadInstr<int>& loperand = dynamic_cast<const LiteralReadInstr<int>&>(lss_instr.loperand_ref());
  const BasicReadInstr<int>& roperand = dynamic_cast<const BasicReadInstr<int>&>(lss_instr.roperand_ref());

  EXPECT_EQ(3, loperand.literal());

  // Make no assumption about the order in which operands are evaluated
  const size_t right_id = roperand.event_ptr()->event_id();
  EXPECT_EQ(2*14, right_id);
}

TEST(ConcurrencyTest, AllocLocalVar) {
  init_recorder();
  Event::reset_id(42);

  const LocalVar<int> var;

  const size_t write_event_id = 2*42+1;
  const DirectWriteEvent<int>& write_event = var.direct_write_event_ref();
  EXPECT_EQ(write_event_id, write_event.event_id());

  const LiteralReadInstr<int>& zero_instr = static_cast<const LiteralReadInstr<int>&>(write_event.instr_ref());
  EXPECT_EQ(0, zero_instr.literal());

  std::unique_ptr<ReadInstr<int>> read_instr_ptr(alloc_read_instr(var));

  const BasicReadInstr<int>& basic_read_instr =
    dynamic_cast<const BasicReadInstr<int>&>(*read_instr_ptr);
  EXPECT_EQ(write_event_id, basic_read_instr.event_ptr()->event_id());
}

TEST(ConcurrencyTest, LocalVarScalarAssignmentWithoutCondition) {
  init_recorder();
  Event::reset_id(7);

  LocalVar<short> var;
  const size_t write_event_id = 2*7+1;
  EXPECT_EQ(write_event_id, var.direct_write_event_ref().event_id());
  EXPECT_EQ(write_event_id, var.read_event_ptr()->event_id());

  var = 5;

  const LiteralReadInstr<short>& read_instr = dynamic_cast<const LiteralReadInstr<short>&>(var.direct_write_event_ref().instr_ref());

  const size_t new_write_event_id = 2*8+1;

  EXPECT_EQ(5, read_instr.literal());
  EXPECT_FALSE(var.addr().is_shared());
  EXPECT_EQ(new_write_event_id, var.direct_write_event_ref().event_id());
  EXPECT_EQ(new_write_event_id, var.read_event_ptr()->event_id());
}

TEST(ConcurrencyTest, LocalVarAssignmentWithoutCondition) {
  init_recorder();
  Event::reset_id(7);

  LocalVar<char> char_integer;
  LocalVar<long> long_integer;

  const size_t char_read_event_id = 2*7+1;
  EXPECT_EQ(char_read_event_id, char_integer.read_event_ptr()->event_id());

  long_integer = 3L + char_integer;

  const BinaryReadInstr<ADD, long, char>& add_instr =
    dynamic_cast<const BinaryReadInstr<ADD, long, char>&>(long_integer.direct_write_event_ref().instr_ref());
  const LiteralReadInstr<long>& loperand = dynamic_cast<const LiteralReadInstr<long>&>(add_instr.loperand_ref());
  const BasicReadInstr<char>& roperand = dynamic_cast<const BasicReadInstr<char>&>(add_instr.roperand_ref());

  EXPECT_EQ(3L, loperand.literal());
  EXPECT_EQ(char_read_event_id, roperand.event_ptr()->event_id());
}

TEST(ConcurrencyTest, LocalVarOtherAssignmentWithoutCondition) {
  init_recorder();
  Event::reset_id(12);

  LocalVar<char> char_integer;
  LocalVar<long> long_integer;
  LocalVar<long> another_long_integer;

  const size_t char_integer_event_id = 2*12+1;
  const size_t long_integer_event_id = 2*13+1;
  const size_t another_long_integer_event_id = 2*14+1;

  EXPECT_EQ(char_integer_event_id, char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(char_integer_event_id, char_integer.read_event_ptr()->event_id());

  EXPECT_EQ(long_integer_event_id, long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(long_integer_event_id, long_integer.read_event_ptr()->event_id());

  EXPECT_EQ(another_long_integer_event_id, another_long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(another_long_integer_event_id, another_long_integer.read_event_ptr()->event_id());

  // See also LocalVarAssignmentWithoutCondition
  long_integer = 3L + char_integer;

  EXPECT_EQ(char_integer_event_id, char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(char_integer_event_id, char_integer.read_event_ptr()->event_id());

  EXPECT_EQ(2*15+1, long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(2*15+1, long_integer.read_event_ptr()->event_id());

  EXPECT_EQ(another_long_integer_event_id, another_long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(another_long_integer_event_id, another_long_integer.read_event_ptr()->event_id());

  another_long_integer = long_integer;

  EXPECT_EQ(char_integer_event_id, char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(char_integer_event_id, char_integer.read_event_ptr()->event_id());

  EXPECT_EQ(2*15+1, long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(2*15+1, long_integer.read_event_ptr()->event_id());

  EXPECT_EQ(2*16+1, another_long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(2*16+1, another_long_integer.read_event_ptr()->event_id());

  const BasicReadInstr<long>& read_instr = dynamic_cast<const BasicReadInstr<long>&>(another_long_integer.direct_write_event_ref().instr_ref());
  EXPECT_EQ(2*15+1, read_instr.event_ptr()->event_id());
  EXPECT_EQ(read_instr.event_ptr(), long_integer.read_event_ptr());
}

TEST(ConcurrencyTest, OverwriteLocalVarArrayElementWithReadInstrPointer) {
  init_recorder();
  Event::reset_id(12);

  LocalVar<char[5]> array_var;

  EXPECT_EQ(2*12+1, array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(2*12+1, array_var.read_event_ptr()->event_id());

  array_var[2] = std::unique_ptr<ReadInstr<char>>(new LiteralReadInstr<char>('Z'));

  EXPECT_EQ(2*12+1, array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(2*13+1, array_var.indirect_write_event_ref().event_id());
  EXPECT_EQ(2*13+1, array_var.read_event_ptr()->event_id());

  const LiteralReadInstr<char>& read_instr = dynamic_cast<const LiteralReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ('Z', read_instr.literal());
}

TEST(ConcurrencyTest, OverwriteLocalVarArrayElementWithLiteral) {
  init_recorder();
  Event::reset_id(12);

  LocalVar<char[5]> array_var;

  EXPECT_EQ(2*12+1, array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(2*12+1, array_var.read_event_ptr()->event_id());

  array_var[2] = 'Z';

  EXPECT_EQ(2*12+1, array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(2*13+1, array_var.indirect_write_event_ref().event_id());
  EXPECT_EQ(2*13+1, array_var.read_event_ptr()->event_id());

  const LiteralReadInstr<char>& read_instr = dynamic_cast<const LiteralReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ('Z', read_instr.literal());
}

TEST(ConcurrencyTest, OverwriteLocalVarArrayElementWithVar) {
  init_recorder();
  Event::reset_id(12);

  LocalVar<char> var;

  EXPECT_EQ(2*12+1, var.direct_write_event_ref().event_id());
  EXPECT_EQ(2*12+1, var.read_event_ptr()->event_id());

  LocalVar<char[5]> array_var;

  EXPECT_EQ(2*13+1, array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(2*13+1, array_var.read_event_ptr()->event_id());

  array_var[2] = var;

  EXPECT_EQ(2*13+1, array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(2*14+1, array_var.indirect_write_event_ref().event_id());
  EXPECT_EQ(2*14+1, array_var.read_event_ptr()->event_id());

  const BasicReadInstr<char>& read_instr = dynamic_cast<const BasicReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ(2*12+1, read_instr.event_ptr()->event_id());
}

TEST(ConcurrencyTest, LocalRecorder) {
  init_recorder();
  const unsigned thread_id = 3;

  push_recorder(thread_id);

  LocalVar<char> var;
  LocalVar<char[5]> array_var;

  array_var[2] = static_cast<char>(0xa2);
  array_var[4] = var;

  Z3 z3;
  Z3ValueEncoder encoder;
  recorder_ptr()->encode(encoder, z3);

  std::stringstream out;
  out << z3.solver;
  EXPECT_EQ("(solver\n  (= k!7 (store k!5 #x0000000000000004 k!1))\n"
            "  (= k!5 (store k!3 #x0000000000000002 #xa2))\n"
            "  (= k!3 ((as const (Array (_ BitVec 64) (_ BitVec 8))) #x00))\n"
            "  (= k!1 #x00))", out.str());
}

TEST(ConcurrencyTest, AllocSharedVar) {
  init_recorder();
  Event::reset_id(42);

  const SharedVar<int> var;

  const DirectWriteEvent<int>& write_event = var.direct_write_event_ref();
  EXPECT_EQ(2*42+1, write_event.event_id());

  const LiteralReadInstr<int>& zero_instr = static_cast<const LiteralReadInstr<int>&>(write_event.instr_ref());
  EXPECT_EQ(0, zero_instr.literal());

  std::unique_ptr<ReadInstr<int>> read_instr_ptr(alloc_read_instr(var));

  const BasicReadInstr<int>& basic_read_instr =
    dynamic_cast<const BasicReadInstr<int>&>(*read_instr_ptr);
  EXPECT_EQ(2*43, basic_read_instr.event_ptr()->event_id());
}

TEST(ConcurrencyTest, SharedVarScalarAssignmentWithoutCondition) {
  init_recorder();
  Event::reset_id(7);

  SharedVar<short> var;
  EXPECT_EQ(2*7+1, var.direct_write_event_ref().event_id());

  var = 5;

  const LiteralReadInstr<short>& read_instr = dynamic_cast<const LiteralReadInstr<short>&>(var.direct_write_event_ref().instr_ref());

  EXPECT_EQ(5, read_instr.literal());
  EXPECT_TRUE(var.addr().is_shared());
  EXPECT_EQ(2*8+1, var.direct_write_event_ref().event_id());
}

TEST(ConcurrencyTest, SharedVarAssignmentWithoutCondition) {
  init_recorder();
  Event::reset_id(7);

  SharedVar<char> char_integer;
  SharedVar<long> long_integer;

  EXPECT_EQ(2*7+1, char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(2*8+1, long_integer.direct_write_event_ref().event_id());

  long_integer = 3L + char_integer;

  EXPECT_EQ(2*7+1, char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(2*10+1, long_integer.direct_write_event_ref().event_id());

  const BinaryReadInstr<ADD, long, char>& add_instr =
    dynamic_cast<const BinaryReadInstr<ADD, long, char>&>(long_integer.direct_write_event_ref().instr_ref());
  const LiteralReadInstr<long>& loperand = dynamic_cast<const LiteralReadInstr<long>&>(add_instr.loperand_ref());
  const BasicReadInstr<char>& roperand = dynamic_cast<const BasicReadInstr<char>&>(add_instr.roperand_ref());

  EXPECT_EQ(3L, loperand.literal());
  EXPECT_EQ(2*9, roperand.event_ptr()->event_id());
}

TEST(ConcurrencyTest, SharedVarOtherAssignmentWithoutCondition) {
  init_recorder();
  Event::reset_id(12);

  SharedVar<char> char_integer;
  SharedVar<long> long_integer;
  SharedVar<long> another_long_integer;

  EXPECT_EQ(2*12+1, char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(2*13+1, long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(2*14+1, another_long_integer.direct_write_event_ref().event_id());

  // See also SharedVarAssignmentWithoutCondition
  long_integer = 3L + char_integer;

  EXPECT_EQ(2*12+1, char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(2*16+1, long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(2*14+1, another_long_integer.direct_write_event_ref().event_id());

  another_long_integer = long_integer;

  EXPECT_EQ(2*12+1, char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(2*16+1, long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(2*18+1, another_long_integer.direct_write_event_ref().event_id());

  const BasicReadInstr<long>& read_instr = dynamic_cast<const BasicReadInstr<long>&>(another_long_integer.direct_write_event_ref().instr_ref());
  EXPECT_EQ(2*17, read_instr.event_ptr()->event_id());
}

TEST(ConcurrencyTest, OverwriteSharedVarArrayElementWithReadInstrPointer) {
  init_recorder();
  Event::reset_id(12);

  SharedVar<char[5]> array_var;

  EXPECT_EQ(2*12+1, array_var.direct_write_event_ref().event_id());

  array_var[2] = std::unique_ptr<ReadInstr<char>>(new LiteralReadInstr<char>('Z'));

  EXPECT_EQ(2*12+1, array_var.direct_write_event_ref().event_id());

  const BasicReadInstr<char[5]>& array_read_instr = static_cast<const BasicReadInstr<char[5]>&>(array_var.indirect_write_event_ref().deref_instr_ref().memory_ref());
  EXPECT_EQ(2*13, array_read_instr.event_ptr()->event_id());

  EXPECT_EQ(2*14+1, array_var.indirect_write_event_ref().event_id());

  const LiteralReadInstr<char>& read_instr = dynamic_cast<const LiteralReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ('Z', read_instr.literal());
}

TEST(ConcurrencyTest, OverwriteSharedVarArrayElementWithLiteral) {
  init_recorder();
  Event::reset_id(12);

  SharedVar<char[5]> array_var;

  EXPECT_EQ(2*12+1, array_var.direct_write_event_ref().event_id());

  array_var[2] = 'Z';

  EXPECT_EQ(2*12+1, array_var.direct_write_event_ref().event_id());

  const BasicReadInstr<char[5]>& array_read_instr = static_cast<const BasicReadInstr<char[5]>&>(array_var.indirect_write_event_ref().deref_instr_ref().memory_ref());
  EXPECT_EQ(2*13, array_read_instr.event_ptr()->event_id());

  EXPECT_EQ(2*14+1, array_var.indirect_write_event_ref().event_id());

  const LiteralReadInstr<char>& read_instr = dynamic_cast<const LiteralReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ('Z', read_instr.literal());
}

TEST(ConcurrencyTest, OverwriteSharedVarArrayElementWithVar) {
  init_recorder();
  Event::reset_id(12);

  SharedVar<char> var;

  EXPECT_EQ(2*12+1, var.direct_write_event_ref().event_id());

  SharedVar<char[5]> array_var;

  EXPECT_EQ(2*13+1, array_var.direct_write_event_ref().event_id());

  array_var[2] = var;

  EXPECT_EQ(2*13+1, array_var.direct_write_event_ref().event_id());

  const BasicReadInstr<char[5]>& array_read_instr = static_cast<const BasicReadInstr<char[5]>&>(array_var.indirect_write_event_ref().deref_instr_ref().memory_ref());
  EXPECT_EQ(2*14, array_read_instr.event_ptr()->event_id());

  EXPECT_EQ(2*16+1, array_var.indirect_write_event_ref().event_id());

  const BasicReadInstr<char>& read_instr = dynamic_cast<const BasicReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ(2*15, read_instr.event_ptr()->event_id());
}

TEST(ConcurrencyTest, SharedRecorder) {
  init_recorder();
  Event::reset_id();
  const unsigned thread_id = 3;

  push_recorder(thread_id);

  SharedVar<char> var;
  SharedVar<char[5]> array_var;

  array_var[2] = static_cast<char>(0xa2);
  array_var[4] = var;

  Z3 z3;
  Z3ValueEncoder encoder;
  recorder_ptr()->encode(encoder, z3);

  std::stringstream out;
  out << z3.solver;
  EXPECT_EQ("(solver\n  (= k!13 (store k!8 #x0000000000000004 k!10))\n"
            "  (= k!7 (store k!4 #x0000000000000002 #xa2))\n"
            "  (= k!3 ((as const (Array (_ BitVec 64) (_ BitVec 8))) #x00))\n"
            "  (= k!1 #x00))", out.str());
}
