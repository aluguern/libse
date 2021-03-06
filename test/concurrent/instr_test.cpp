#include <sstream>

#include "concurrent/instr.h"
#include "concurrent/encoder_c0.h"

#include "gtest/gtest.h"

using namespace se;

#define READ_EVENT_ID(id) (id)
#define WRITE_EVENT_ID(id) (id)

TEST(InstrTest, LiteralReadInstrWithCondition) {
  Event::reset_id(7);

  const unsigned thread_id = 3;
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  int literal = 31516;
  const LiteralReadInstr<int> instr(literal, condition_ptr);
  EXPECT_EQ(literal, instr.literal());
}

TEST(InstrTest, LiteralReadInstrInitializer) {
  const LiteralReadInstr<unsigned long> ulong_instr;
  EXPECT_EQ(0UL, ulong_instr.literal());
}

TEST(InstrTest, LiteralReadInstrWithoutCondition) {
  const LiteralReadInstr<unsigned long> instr(0UL);
  EXPECT_EQ(0UL, instr.literal());
}

TEST(InstrTest, BasicReadInstrWithoutCondition) {
  Event::reset_id(4);

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, zone));

  const BasicReadInstr<int> instr(std::move(event_ptr));
  EXPECT_EQ(READ_EVENT_ID(4), instr.event_ptr()->event_id());
}

TEST(InstrTest, BasicReadInstrWithCondition) {
  Event::reset_id(7);

  const unsigned thread_id = 3;
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, zone, condition_ptr));

  const BasicReadInstr<int> instr(std::move(event_ptr));
  EXPECT_EQ(READ_EVENT_ID(8), instr.event_ptr()->event_id());
}

TEST(InstrTest, UnaryReadInstrWithoutCondition) {
  Event::reset_id(7);

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, zone));
  std::unique_ptr<ReadInstr<int>> basic_read_instr(new BasicReadInstr<int>(std::move(event_ptr)));
  const UnaryReadInstr<NOT, int> instr(std::move(basic_read_instr));

  const BasicReadInstr<int>& operand = dynamic_cast<const BasicReadInstr<int>&>(instr.operand_ref());
  EXPECT_EQ(READ_EVENT_ID(7), operand.event_ptr()->event_id());
}

TEST(InstrTest, UnaryReadInstrWithCondition) {
  Event::reset_id(7);

  const unsigned thread_id = 3;
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, zone, condition_ptr));
  std::unique_ptr<ReadInstr<int>> basic_read_instr(new BasicReadInstr<int>(std::move(event_ptr)));
  const UnaryReadInstr<NOT, int> instr(std::move(basic_read_instr));

  const BasicReadInstr<int>& operand = dynamic_cast<const BasicReadInstr<int>&>(instr.operand_ref());
  EXPECT_EQ(READ_EVENT_ID(8), operand.event_ptr()->event_id());
}

TEST(InstrTest, BinaryReadInstrWithoutCondition) {
  Event::reset_id(7);

  const unsigned thread_id = thread_id;
  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = Zone::unique_atom();

  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(thread_id, zone_a));
  std::unique_ptr<ReadEvent<long>> event_ptr_b(new ReadEvent<long>(thread_id, zone_b));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));
  const BinaryReadInstr<ADD, int, long> instr(std::move(basic_read_instr_a) /* explicit move */,
    std::unique_ptr<ReadInstr<long>>(new BasicReadInstr<long>(std::move(event_ptr_b))) /* implicit move */ );

  const BasicReadInstr<int>& left_child = dynamic_cast<const BasicReadInstr<int>&>(instr.loperand_ref());
  const BasicReadInstr<long>& right_child = dynamic_cast<const BasicReadInstr<long>&>(instr.roperand_ref());
  EXPECT_EQ(READ_EVENT_ID(7), left_child.event_ptr()->event_id());

  EXPECT_EQ(READ_EVENT_ID(8), right_child.event_ptr()->event_id());
}

TEST(InstrTest, BinaryReadInstrWithCondition) {
  Event::reset_id(7);

  const unsigned thread_id = 3;
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = Zone::unique_atom();
  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(thread_id, zone_a, condition_ptr));
  std::unique_ptr<ReadEvent<long>> event_ptr_b(new ReadEvent<long>(thread_id, zone_b, condition_ptr));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));
  const BinaryReadInstr<ADD, int, long> instr(std::move(basic_read_instr_a) /* explicit move */,
    std::unique_ptr<ReadInstr<long>>(new BasicReadInstr<long>(std::move(event_ptr_b))) /* implicit move */ );

  const BasicReadInstr<int>& left_child = dynamic_cast<const BasicReadInstr<int>&>(instr.loperand_ref());
  const BasicReadInstr<long>& right_child = dynamic_cast<const BasicReadInstr<long>&>(instr.roperand_ref());
  EXPECT_EQ(READ_EVENT_ID(8), left_child.event_ptr()->event_id());

  EXPECT_EQ(READ_EVENT_ID(9), right_child.event_ptr()->event_id());
}

// The library forbids a binary operator to be constructed from distinct conditions.
TEST(InstrTest, BinaryReadInstrWithDifferentPathConditions) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";

  const unsigned thread_id = 3;
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr_a(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr_a(
    new BasicReadInstr<bool>(std::move(condition_event_ptr_a)));

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr_b(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr_b(
    new BasicReadInstr<bool>(std::move(condition_event_ptr_b)));

  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = Zone::unique_atom();
  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(thread_id, zone_a, condition_ptr_a));
  std::unique_ptr<ReadEvent<long>> event_ptr_b(new ReadEvent<long>(thread_id, zone_b, condition_ptr_b));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));
  std::unique_ptr<ReadInstr<long>> basic_read_instr_b(new BasicReadInstr<long>(std::move(event_ptr_b)));
  EXPECT_EXIT((BinaryReadInstr<ADD, int, long>(std::move(basic_read_instr_a),
    std::move(basic_read_instr_b))), ::testing::KilledBySignal(SIGABRT), "Assertion");
}

TEST(InstrTest, DereferenceFixedSizeArrayReadInstrWithoutCondition) {
  Event::reset_id(7);

  const unsigned thread_id = 3;
  const size_t array_size = 5;

  const Zone offset_zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<size_t>> offset_event_ptr(new ReadEvent<size_t>(thread_id, offset_zone));
  std::unique_ptr<ReadInstr<size_t>> offset_read_instr(new BasicReadInstr<size_t>(std::move(offset_event_ptr)));

  const Zone pointer_zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<char[array_size]>> pointer_event_ptr(new ReadEvent<char[array_size]>(thread_id, pointer_zone));
  std::unique_ptr<ReadInstr<char[array_size]>> pointer_read_instr(new BasicReadInstr<char[array_size]>(std::move(pointer_event_ptr)));

  const DerefReadInstr<char[array_size], size_t> dereference_instr(std::move(pointer_read_instr), std::move(offset_read_instr));

  const BasicReadInstr<size_t>& offset_instr = dynamic_cast<const BasicReadInstr<size_t>&>(dereference_instr.offset_ref());
  const BasicReadInstr<char[array_size]>& memory_instr = dynamic_cast<const BasicReadInstr<char[array_size]>&>(dereference_instr.memory_ref());
  EXPECT_EQ(READ_EVENT_ID(7), offset_instr.event_ptr()->event_id());
  EXPECT_EQ(READ_EVENT_ID(8), memory_instr.event_ptr()->event_id());
}

TEST(InstrTest, DereferenceFixedSizeArrayReadInstrWithCondition) {
  Event::reset_id(7);

  const unsigned thread_id = 3;
  const size_t array_size = 5;
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  const Zone offset_zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<size_t>> offset_event_ptr(new ReadEvent<size_t>(thread_id, offset_zone, condition_ptr));
  std::unique_ptr<ReadInstr<size_t>> offset_read_instr(new BasicReadInstr<size_t>(std::move(offset_event_ptr)));

  const Zone pointer_zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<char[array_size]>> pointer_event_ptr(new ReadEvent<char[array_size]>(thread_id, pointer_zone, condition_ptr));
  std::unique_ptr<ReadInstr<char[array_size]>> pointer_read_instr(new BasicReadInstr<char[array_size]>(std::move(pointer_event_ptr)));

  const DerefReadInstr<char[array_size], size_t> dereference_instr(std::move(pointer_read_instr), std::move(offset_read_instr));

  const BasicReadInstr<size_t>& offset_instr = dynamic_cast<const BasicReadInstr<size_t>&>(dereference_instr.offset_ref());
  const BasicReadInstr<char[array_size]>& memory_instr = dynamic_cast<const BasicReadInstr<char[array_size]>&>(dereference_instr.memory_ref());
  EXPECT_EQ(READ_EVENT_ID(8), offset_instr.event_ptr()->event_id());
  EXPECT_EQ(READ_EVENT_ID(9), memory_instr.event_ptr()->event_id());
}

TEST(InstrTest, ReadInstrResult) {
  static_assert(std::is_same<long, ReadInstrResult<LiteralReadInstr<long>>::Type>::value, "Wrong result type");
  static_assert(std::is_same<size_t, ReadInstrResult<BasicReadInstr<size_t>>::Type>::value, "Wrong result type");
  static_assert(std::is_same<bool, ReadInstrResult<UnaryReadInstr<NOT, long>>::Type>::value, "Wrong result type");
  static_assert(std::is_same<long, ReadInstrResult<BinaryReadInstr<ADD, int, long>>::Type>::value, "Wrong result type");
  static_assert(std::is_same<char, ReadInstrResult<DerefReadInstr<char[5], size_t>>::Type>::value, "Wrong result type");
}

TEST(InstrTest, Filter) {
  Event::reset_id(7);

  const unsigned thread_id = 3;
  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = Zone::unique_atom();
  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(thread_id, zone_a));
  std::unique_ptr<ReadEvent<long>> event_ptr_b(new ReadEvent<long>(thread_id, zone_b));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));
  const BinaryReadInstr<ADD, int, long> instr(std::move(basic_read_instr_a) /* explicit move */,
    std::unique_ptr<ReadInstr<long>>(new BasicReadInstr<long>(std::move(event_ptr_b))) /* implicit move */ );

  std::forward_list<std::shared_ptr<Event>> event_ptrs;
  instr.filter(event_ptrs);

  const ReadEvent<int>& event_a = dynamic_cast<const ReadEvent<int>&>(*event_ptrs.front());
  EXPECT_EQ(READ_EVENT_ID(7), event_a.event_id());

  event_ptrs.pop_front();

  const ReadEvent<long>& event_b = dynamic_cast<const ReadEvent<long>&>(*event_ptrs.front());
  EXPECT_EQ(READ_EVENT_ID(8), event_b.event_id());

  event_ptrs.pop_front();
  EXPECT_TRUE(event_ptrs.empty());
}

class ReadInstrPrinter : public ReadInstrSwitch<ReadInstrPrinter, std::ostream> {
public:

  ReadInstrPrinter() {}

  template<typename T>
  void case_instr(const LiteralReadInstr<T>& instr, std::ostream& out) const {
    out << "LiteralReadInstr" << std::endl;
  }

  void case_instr(const LiteralReadInstr<long>& instr, std::ostream& out) const {
    out << "LiteralReadInstr<long>" << std::endl;
  }

};

TEST(InstrTest, ReadInstrSwitch) {
  const LiteralReadInstr<int> int_literal_read_instr(5);
  const LiteralReadInstr<long> long_literal_read_instr(7L);

  unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(thread_id, zone));
  const BasicReadInstr<long> basic_read_instr(std::move(event_ptr));
  
  const ReadInstrPrinter printer;
  std::stringstream out;

  printer.switch_instr(int_literal_read_instr, out);
  printer.switch_instr(basic_read_instr, out);
  printer.switch_instr(long_literal_read_instr, out);

  EXPECT_EQ("LiteralReadInstr\nLiteralReadInstr<long>\n", out.str());
}
