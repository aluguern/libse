#include "gtest/gtest.h"

#include "concurrent.h"
#include "concurrent/zone.h"

using namespace se;

#define READ_EVENT_ID(id) (id)
#define WRITE_EVENT_ID(id) (id)

TEST(ConcurrencyTest, AllocLiteralReadInstrWithConstant) {
  Threads::reset();
  Threads::begin_main_thread();

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(alloc_read_instr(42L));

  const LiteralReadInstr<long>& literal_read_instr =
    dynamic_cast<const LiteralReadInstr<long>&>(*read_instr_ptr);
  EXPECT_EQ(42L, literal_read_instr.literal());
}

TEST(ConcurrencyTest, AllocLiteralReadInstrWithVariable) {
  Threads::reset();
  Threads::begin_main_thread();

  long literal = 42;
  std::unique_ptr<ReadInstr<long>> read_instr_ptr(alloc_read_instr(literal));

  const LiteralReadInstr<long>& literal_read_instr =
    dynamic_cast<const LiteralReadInstr<long>&>(*read_instr_ptr);
  EXPECT_EQ(literal, literal_read_instr.literal());
}

TEST(ConcurrencyTest, AllocLiteralReadInstrWithCondition) {
  Threads::reset(7);
  Threads::begin_main_thread();

  const unsigned thread_id = ThisThread::thread_id();
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<char>> event_ptr(new ReadEvent<char>(thread_id, zone));
  std::unique_ptr<ReadInstr<char>> basic_read_instr_ptr(new BasicReadInstr<char>(std::move(event_ptr)));

  ThisThread::recorder().begin_then(3L < std::move(basic_read_instr_ptr));

  std::unique_ptr<ReadInstr<long long>> read_instr_ptr(alloc_read_instr(42LL));

  const LiteralReadInstr<long long>& literal_read_instr =
    dynamic_cast<const LiteralReadInstr<long long>&>(*read_instr_ptr);
  EXPECT_EQ(42LL, literal_read_instr.literal());

  const BinaryReadInstr<LSS, long, char>& lss_instr =
    dynamic_cast<const BinaryReadInstr<LSS, long, char>&>(*read_instr_ptr->condition_ptr());

  const LiteralReadInstr<long>& loperand = dynamic_cast<const LiteralReadInstr<long>&>(lss_instr.loperand_ref());
  const BasicReadInstr<char>& roperand = dynamic_cast<const BasicReadInstr<char>&>(lss_instr.roperand_ref());
  EXPECT_EQ(3L, loperand.literal());

  EXPECT_EQ(READ_EVENT_ID(7), roperand.event_ptr()->event_id());
}

TEST(ConcurrencyTest, UnaryOperatorNOTWithCondition) {
  Threads::reset(14);
  Threads::begin_main_thread();

  const unsigned thread_id = ThisThread::thread_id();
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, zone, condition));
  std::unique_ptr<ReadInstr<int>> basic_read_instr(new BasicReadInstr<int>(std::move(event_ptr)));
  std::unique_ptr<ReadInstr<bool>> instr_ptr(! std::move(basic_read_instr));

  const UnaryReadInstr<NOT, int>& instr = dynamic_cast<const UnaryReadInstr<NOT, int>&>(*instr_ptr);

  const BasicReadInstr<int>& operand = dynamic_cast<const BasicReadInstr<int>&>(instr.operand_ref());
  EXPECT_TRUE(READ_EVENT_ID(15) == operand.event_ptr()->event_id());
}

TEST(ConcurrencyTest, BinaryOperatorLSSWithoutCondition) {
  Threads::reset(14);
  Threads::begin_main_thread();

  const unsigned thread_id = ThisThread::thread_id();
  const Zone zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(thread_id, zone));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_ptr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));

  std::unique_ptr<ReadEvent<int>> event_ptr_b(new ReadEvent<int>(thread_id, zone));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_ptr_b(new BasicReadInstr<int>(std::move(event_ptr_b)));

  std::unique_ptr<ReadInstr<bool>> instr_ptr(std::move(basic_read_instr_ptr_a) < std::move(basic_read_instr_ptr_b));

  const BinaryReadInstr<LSS, int, int>& instr =
    dynamic_cast<const BinaryReadInstr<LSS, int, int>&>(*instr_ptr);

  const BasicReadInstr<int>& loperand = dynamic_cast<const BasicReadInstr<int>&>(instr.loperand_ref());
  const BasicReadInstr<int>& roperand = dynamic_cast<const BasicReadInstr<int>&>(instr.roperand_ref());

  // Make no assumption about the order in which operands are evaluated
  const unsigned left_id = loperand.event_ptr()->event_id();
  const unsigned right_id = roperand.event_ptr()->event_id();
  EXPECT_TRUE(READ_EVENT_ID(14) == left_id || READ_EVENT_ID(15) == left_id);
  EXPECT_TRUE(READ_EVENT_ID(14) == right_id || READ_EVENT_ID(15) == right_id);
  EXPECT_NE(left_id, right_id);
}

TEST(ConcurrencyTest, BinaryOperatorADDWithCondition) {
  Threads::reset(14);
  Threads::begin_main_thread();

  const unsigned thread_id = ThisThread::thread_id();
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = Zone::unique_atom();

  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(thread_id, zone_a, condition));
  std::unique_ptr<ReadEvent<long>> event_ptr_b(new ReadEvent<long>(thread_id, zone_b, condition));

  std::unique_ptr<ReadInstr<int>> basic_read_instr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));
  std::unique_ptr<ReadInstr<long>> instr_ptr(std::move(basic_read_instr_a) /* explicit move */ +
    std::unique_ptr<ReadInstr<long>>(new BasicReadInstr<long>(std::move(event_ptr_b))) /* implicit move */);

  const BinaryReadInstr<ADD, int, long>& instr = dynamic_cast<const BinaryReadInstr<ADD, int, long>&>(*instr_ptr);

  const BasicReadInstr<int>& loperand = dynamic_cast<const BasicReadInstr<int>&>(instr.loperand_ref());
  const BasicReadInstr<long>& roperand = dynamic_cast<const BasicReadInstr<long>&>(instr.roperand_ref());

  // Make no assumption about the order in which operands are evaluated
  const unsigned left_id = loperand.event_ptr()->event_id();
  const unsigned right_id = roperand.event_ptr()->event_id();
  EXPECT_TRUE(READ_EVENT_ID(15) == left_id || READ_EVENT_ID(16) == left_id);
  EXPECT_TRUE(READ_EVENT_ID(15) == right_id || READ_EVENT_ID(16) == right_id);
  EXPECT_NE(left_id, right_id);
}

TEST(ConcurrencyTest, BinaryOperatorLSSReadInstrPointerLiteralWithoutCondition) {
  Threads::reset(14);
  Threads::begin_main_thread();

  const unsigned thread_id = ThisThread::thread_id();
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, zone));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_ptr(new BasicReadInstr<int>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<bool>> lss_instr_ptr(std::move(basic_read_instr_ptr) < 3);

  const BinaryReadInstr<LSS, int, int>& lss_instr =
    dynamic_cast<const BinaryReadInstr<LSS, int, int>&>(*lss_instr_ptr);

  const BasicReadInstr<int>& loperand = dynamic_cast<const BasicReadInstr<int>&>(lss_instr.loperand_ref());
  const LiteralReadInstr<int>& roperand = dynamic_cast<const LiteralReadInstr<int>&>(lss_instr.roperand_ref());

  // Make no assumption about the order in which operands are evaluated
  const unsigned left_id = loperand.event_ptr()->event_id();
  EXPECT_EQ(READ_EVENT_ID(14), left_id);

  EXPECT_EQ(3, roperand.literal());
}

TEST(ConcurrencyTest, BinaryOperatorLSSLiteralReadInstrPointerWithoutCondition) {
  Threads::reset(14);
  Threads::begin_main_thread();

  const unsigned thread_id = ThisThread::thread_id();
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, zone));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_ptr(new BasicReadInstr<int>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<bool>> lss_instr_ptr(3 < std::move(basic_read_instr_ptr));

  const BinaryReadInstr<LSS, int, int>& lss_instr =
    dynamic_cast<const BinaryReadInstr<LSS, int, int>&>(*lss_instr_ptr);

  const LiteralReadInstr<int>& loperand = dynamic_cast<const LiteralReadInstr<int>&>(lss_instr.loperand_ref());
  const BasicReadInstr<int>& roperand = dynamic_cast<const BasicReadInstr<int>&>(lss_instr.roperand_ref());

  EXPECT_EQ(3, loperand.literal());

  // Make no assumption about the order in which operands are evaluated
  const unsigned right_id = roperand.event_ptr()->event_id();
  EXPECT_EQ(READ_EVENT_ID(14), right_id);
}

TEST(ConcurrencyTest, AllocLocalVar) {
  Threads::reset(42);
  Threads::begin_main_thread();

  const LocalVar<int> var;

  const unsigned write_event_id = WRITE_EVENT_ID(42);
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
  Threads::reset(7);
  Threads::begin_main_thread();

  LocalVar<short> var;
  const unsigned write_event_id = WRITE_EVENT_ID(7);
  EXPECT_EQ(write_event_id, var.direct_write_event_ref().event_id());
  EXPECT_EQ(write_event_id, var.read_event_ptr()->event_id());

  var = 5;

  const LiteralReadInstr<short>& read_instr = dynamic_cast<const LiteralReadInstr<short>&>(var.direct_write_event_ref().instr_ref());

  const unsigned new_write_event_id = WRITE_EVENT_ID(8);

  EXPECT_EQ(5, read_instr.literal());
  EXPECT_TRUE(var.zone().is_bottom());
  EXPECT_EQ(new_write_event_id, var.direct_write_event_ref().event_id());
  EXPECT_EQ(new_write_event_id, var.read_event_ptr()->event_id());
}

TEST(ConcurrencyTest, LocalVarAssignmentWithoutCondition) {
  Threads::reset(7);
  Threads::begin_main_thread();

  LocalVar<char> char_integer;
  LocalVar<long> long_integer;

  const unsigned char_read_event_id = WRITE_EVENT_ID(7);
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
  Threads::reset(12);
  Threads::begin_main_thread();

  LocalVar<char> char_integer;
  LocalVar<long> long_integer;
  LocalVar<long> another_long_integer;

  const unsigned char_integer_event_id = WRITE_EVENT_ID(12);
  const unsigned long_integer_event_id = WRITE_EVENT_ID(13);
  const unsigned another_long_integer_event_id = WRITE_EVENT_ID(14);

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

  EXPECT_EQ(WRITE_EVENT_ID(15), long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(15), long_integer.read_event_ptr()->event_id());

  EXPECT_EQ(another_long_integer_event_id, another_long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(another_long_integer_event_id, another_long_integer.read_event_ptr()->event_id());

  another_long_integer = long_integer;

  EXPECT_EQ(char_integer_event_id, char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(char_integer_event_id, char_integer.read_event_ptr()->event_id());

  EXPECT_EQ(WRITE_EVENT_ID(15), long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(15), long_integer.read_event_ptr()->event_id());

  EXPECT_EQ(WRITE_EVENT_ID(16), another_long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(16), another_long_integer.read_event_ptr()->event_id());

  const BasicReadInstr<long>& read_instr = dynamic_cast<const BasicReadInstr<long>&>(another_long_integer.direct_write_event_ref().instr_ref());
  EXPECT_EQ(WRITE_EVENT_ID(15), read_instr.event_ptr()->event_id());
  EXPECT_EQ(read_instr.event_ptr(), long_integer.read_event_ptr());
}

TEST(ConcurrencyTest, OverwriteLocalVarArrayElementWithReadInstrPointer) {
  Threads::reset(12);
  Threads::begin_main_thread();

  LocalVar<char[5]> array_var;

  EXPECT_EQ(WRITE_EVENT_ID(12), array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(12), array_var.read_event_ptr()->event_id());

  array_var[2] = std::unique_ptr<ReadInstr<char>>(new LiteralReadInstr<char>('Z'));

  EXPECT_EQ(WRITE_EVENT_ID(12), array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(13), array_var.indirect_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(13), array_var.read_event_ptr()->event_id());

  const LiteralReadInstr<char>& read_instr = dynamic_cast<const LiteralReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ('Z', read_instr.literal());
}

TEST(ConcurrencyTest, OverwriteLocalVarArrayElementWithLiteral) {
  Threads::reset(12);
  Threads::begin_main_thread();

  LocalVar<char[5]> array_var;

  EXPECT_EQ(WRITE_EVENT_ID(12), array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(12), array_var.read_event_ptr()->event_id());

  array_var[2] = 'Z';

  EXPECT_EQ(WRITE_EVENT_ID(12), array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(13), array_var.indirect_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(13), array_var.read_event_ptr()->event_id());

  const LiteralReadInstr<char>& read_instr = dynamic_cast<const LiteralReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ('Z', read_instr.literal());
}

TEST(ConcurrencyTest, OverwriteLocalVarArrayElementWithVar) {
  Threads::reset(12);
  Threads::begin_main_thread();

  LocalVar<char> var;

  EXPECT_EQ(WRITE_EVENT_ID(12), var.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(12), var.read_event_ptr()->event_id());

  LocalVar<char[5]> array_var;

  EXPECT_EQ(WRITE_EVENT_ID(13), array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(13), array_var.read_event_ptr()->event_id());

  array_var[2] = var;

  EXPECT_EQ(WRITE_EVENT_ID(13), array_var.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(14), array_var.indirect_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(14), array_var.read_event_ptr()->event_id());

  const BasicReadInstr<char>& read_instr = dynamic_cast<const BasicReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ(WRITE_EVENT_ID(12), read_instr.event_ptr()->event_id());
}

TEST(ConcurrencyTest, AllocSharedVar) {
  Threads::reset(42);
  Threads::begin_main_thread();

  const SharedVar<int> var;

  const DirectWriteEvent<int>& write_event = var.direct_write_event_ref();
  EXPECT_EQ(WRITE_EVENT_ID(42), write_event.event_id());

  const LiteralReadInstr<int>& zero_instr = static_cast<const LiteralReadInstr<int>&>(write_event.instr_ref());
  EXPECT_EQ(0, zero_instr.literal());

  std::unique_ptr<ReadInstr<int>> read_instr_ptr(alloc_read_instr(var));

  const BasicReadInstr<int>& basic_read_instr =
    dynamic_cast<const BasicReadInstr<int>&>(*read_instr_ptr);
  EXPECT_EQ(READ_EVENT_ID(43), basic_read_instr.event_ptr()->event_id());
}

TEST(ConcurrencyTest, SharedVarScalarAssignmentWithoutCondition) {
  Threads::reset(7);
  Threads::begin_main_thread();

  SharedVar<short> var;
  EXPECT_EQ(WRITE_EVENT_ID(7), var.direct_write_event_ref().event_id());

  var = 5;

  const LiteralReadInstr<short>& read_instr = dynamic_cast<const LiteralReadInstr<short>&>(var.direct_write_event_ref().instr_ref());

  EXPECT_EQ(5, read_instr.literal());
  EXPECT_FALSE(var.zone().is_bottom());
  EXPECT_EQ(WRITE_EVENT_ID(8), var.direct_write_event_ref().event_id());
}

TEST(ConcurrencyTest, SharedVarAssignmentWithoutCondition) {
  Threads::reset(7);
  Threads::begin_main_thread();

  SharedVar<char> char_integer;
  SharedVar<long> long_integer;

  EXPECT_EQ(WRITE_EVENT_ID(7), char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(8), long_integer.direct_write_event_ref().event_id());

  long_integer = 3L + char_integer;

  EXPECT_EQ(WRITE_EVENT_ID(7), char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(10), long_integer.direct_write_event_ref().event_id());

  const BinaryReadInstr<ADD, long, char>& add_instr =
    dynamic_cast<const BinaryReadInstr<ADD, long, char>&>(long_integer.direct_write_event_ref().instr_ref());
  const LiteralReadInstr<long>& loperand = dynamic_cast<const LiteralReadInstr<long>&>(add_instr.loperand_ref());
  const BasicReadInstr<char>& roperand = dynamic_cast<const BasicReadInstr<char>&>(add_instr.roperand_ref());

  EXPECT_EQ(3L, loperand.literal());
  EXPECT_EQ(READ_EVENT_ID(9), roperand.event_ptr()->event_id());
}

TEST(ConcurrencyTest, SharedVarOtherAssignmentWithoutCondition) {
  Threads::reset(12);
  Threads::begin_main_thread();

  SharedVar<char> char_integer;
  SharedVar<long> long_integer;
  SharedVar<long> another_long_integer;

  EXPECT_EQ(WRITE_EVENT_ID(12), char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(13), long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(14), another_long_integer.direct_write_event_ref().event_id());

  // See also SharedVarAssignmentWithoutCondition
  long_integer = 3L + char_integer;

  EXPECT_EQ(WRITE_EVENT_ID(12), char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(16), long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(14), another_long_integer.direct_write_event_ref().event_id());

  another_long_integer = long_integer;

  EXPECT_EQ(WRITE_EVENT_ID(12), char_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(16), long_integer.direct_write_event_ref().event_id());
  EXPECT_EQ(WRITE_EVENT_ID(18), another_long_integer.direct_write_event_ref().event_id());

  const BasicReadInstr<long>& read_instr = dynamic_cast<const BasicReadInstr<long>&>(another_long_integer.direct_write_event_ref().instr_ref());
  EXPECT_EQ(READ_EVENT_ID(17), read_instr.event_ptr()->event_id());
}

TEST(ConcurrencyTest, OverwriteSharedVarArrayElementWithReadInstrPointer) {
  Threads::reset(12);
  Threads::begin_main_thread();

  SharedVar<char[5]> array_var;

  EXPECT_EQ(WRITE_EVENT_ID(12), array_var.direct_write_event_ref().event_id());

  array_var[2] = std::unique_ptr<ReadInstr<char>>(new LiteralReadInstr<char>('Z'));

  EXPECT_EQ(WRITE_EVENT_ID(12), array_var.direct_write_event_ref().event_id());

  const BasicReadInstr<char[5]>& array_read_instr = static_cast<const BasicReadInstr<char[5]>&>(array_var.indirect_write_event_ref().deref_instr_ref().memory_ref());
  EXPECT_EQ(READ_EVENT_ID(13), array_read_instr.event_ptr()->event_id());

  EXPECT_EQ(WRITE_EVENT_ID(14), array_var.indirect_write_event_ref().event_id());

  const LiteralReadInstr<char>& read_instr = dynamic_cast<const LiteralReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ('Z', read_instr.literal());
}

TEST(ConcurrencyTest, OverwriteSharedVarArrayElementWithLiteral) {
  Threads::reset(12);
  Threads::begin_main_thread();

  SharedVar<char[5]> array_var;

  EXPECT_EQ(WRITE_EVENT_ID(12), array_var.direct_write_event_ref().event_id());

  array_var[2] = 'Z';

  EXPECT_EQ(WRITE_EVENT_ID(12), array_var.direct_write_event_ref().event_id());

  const BasicReadInstr<char[5]>& array_read_instr = static_cast<const BasicReadInstr<char[5]>&>(array_var.indirect_write_event_ref().deref_instr_ref().memory_ref());
  EXPECT_EQ(READ_EVENT_ID(13), array_read_instr.event_ptr()->event_id());

  EXPECT_EQ(WRITE_EVENT_ID(14), array_var.indirect_write_event_ref().event_id());

  const LiteralReadInstr<char>& read_instr = dynamic_cast<const LiteralReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ('Z', read_instr.literal());
}

TEST(ConcurrencyTest, OverwriteSharedVarArrayElementWithVar) {
  Threads::reset(12);
  Threads::begin_main_thread();

  SharedVar<char> var;

  EXPECT_EQ(WRITE_EVENT_ID(12), var.direct_write_event_ref().event_id());

  SharedVar<char[5]> array_var;

  EXPECT_EQ(WRITE_EVENT_ID(13), array_var.direct_write_event_ref().event_id());

  array_var[2] = var;

  EXPECT_EQ(WRITE_EVENT_ID(13), array_var.direct_write_event_ref().event_id());

  const BasicReadInstr<char[5]>& array_read_instr = static_cast<const BasicReadInstr<char[5]>&>(array_var.indirect_write_event_ref().deref_instr_ref().memory_ref());
  EXPECT_EQ(READ_EVENT_ID(14), array_read_instr.event_ptr()->event_id());

  EXPECT_EQ(WRITE_EVENT_ID(16), array_var.indirect_write_event_ref().event_id());

  const BasicReadInstr<char>& read_instr = dynamic_cast<const BasicReadInstr<char>&>(array_var.indirect_write_event_ref().instr_ref());
  EXPECT_EQ(READ_EVENT_ID(15), read_instr.event_ptr()->event_id());
}

TEST(ConcurrencyTest, LocalArrayOffsetZone) {
  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<long long[3]> a;
  EXPECT_EQ(a[0].zone(), a.zone());
  EXPECT_EQ(a[1].zone(), a.zone());
  EXPECT_EQ(a[2].zone(), a.zone());
}

TEST(ConcurrencyTest, SharedArrayOffsetZone) {
  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<long[3]> a;
  EXPECT_EQ(a[0].zone(), a.zone());
  EXPECT_EQ(a[1].zone(), a.zone());
  EXPECT_EQ(a[2].zone(), a.zone());
}

TEST(ConcurrencyTest, SharedArrayZone) {
  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<long[3]> a;

  EXPECT_FALSE(a[0].zone().meet(a.zone()).is_bottom());
  EXPECT_FALSE(a[1].zone().meet(a.zone()).is_bottom());
  EXPECT_FALSE(a[2].zone().meet(a.zone()).is_bottom());

  EXPECT_FALSE(a[0].zone().meet(a[1].zone()).is_bottom());
  EXPECT_FALSE(a[1].zone().meet(a[2].zone()).is_bottom());
}
