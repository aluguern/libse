#include "gtest/gtest.h"

#include <sstream>

#include "concurrent/instr.h"

using namespace se;

TEST(InstrTest, LiteralReadInstrWithCondition) {
  Event::reset_id(7);
  uintptr_t condition_read_access = 0x03fa;

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  int literal = 31516;
  const LiteralReadInstr<int> instr(literal, condition_ptr);
  EXPECT_EQ(literal, instr.literal());
}

TEST(InstrTest, LiteralReadInstrWithoutCondition) {
  const LiteralReadInstr<unsigned long> instr(0UL);
  EXPECT_EQ(0UL, instr.literal());
}

TEST(InstrTest, BasicReadInstrWithoutCondition) {
  Event::reset_id(3);
  uintptr_t access = 0x03fa;
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(access));

  const BasicReadInstr<int> instr(std::move(event_ptr));
  EXPECT_EQ(3, instr.event_ptr()->id());
  EXPECT_EQ(1, instr.event_ptr()->addr().ptrs().size());
}

TEST(InstrTest, BasicReadInstrWithCondition) {
  Event::reset_id(7);
  uintptr_t condition_read_access = 0x03fa;

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  uintptr_t access = 0x03fa;
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(access, condition_ptr));

  const BasicReadInstr<int> instr(std::move(event_ptr));
  EXPECT_EQ(8, instr.event_ptr()->id());
  EXPECT_EQ(1, instr.event_ptr()->addr().ptrs().size());
}

TEST(InstrTest, UnaryReadInstrWithoutCondition) {
  Event::reset_id(7);
  uintptr_t access = 0x03fa;
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(access));
  std::unique_ptr<ReadInstr<int>> basic_read_instr(new BasicReadInstr<int>(std::move(event_ptr)));
  const UnaryReadInstr<NOT, int> instr(std::move(basic_read_instr));

  const BasicReadInstr<int>& operand = dynamic_cast<const BasicReadInstr<int>&>(instr.operand_ref());
  EXPECT_EQ(7, operand.event_ptr()->id());
  EXPECT_EQ(1, operand.event_ptr()->addr().ptrs().size());
}

TEST(InstrTest, UnaryReadInstrWithCondition) {
  Event::reset_id(7);
  uintptr_t condition_read_access = 0x03fa;

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  uintptr_t access = 0x03fb;
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(access, condition_ptr));
  std::unique_ptr<ReadInstr<int>> basic_read_instr(new BasicReadInstr<int>(std::move(event_ptr)));
  const UnaryReadInstr<NOT, int> instr(std::move(basic_read_instr));

  const BasicReadInstr<int>& operand = dynamic_cast<const BasicReadInstr<int>&>(instr.operand_ref());
  EXPECT_EQ(8, operand.event_ptr()->id());
  EXPECT_EQ(1, operand.event_ptr()->addr().ptrs().size());
}

TEST(InstrTest, BinaryReadInstrWithoutCondition) {
  Event::reset_id(7);
  uintptr_t access = 0x03fa;
  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(access));
  std::unique_ptr<ReadEvent<long>> event_ptr_b(new ReadEvent<long>(access + 1));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));
  const BinaryReadInstr<ADD, int, long> instr(std::move(basic_read_instr_a) /* explicit move */,
    std::unique_ptr<ReadInstr<long>>(new BasicReadInstr<long>(std::move(event_ptr_b))) /* implicit move */ );

  const BasicReadInstr<int>& left_child = dynamic_cast<const BasicReadInstr<int>&>(instr.loperand_ref());
  const BasicReadInstr<long>& right_child = dynamic_cast<const BasicReadInstr<long>&>(instr.roperand_ref());
  EXPECT_EQ(7, left_child.event_ptr()->id());
  EXPECT_EQ(1, left_child.event_ptr()->addr().ptrs().size());

  EXPECT_EQ(8, right_child.event_ptr()->id());
  EXPECT_EQ(1, right_child.event_ptr()->addr().ptrs().size());
}

TEST(InstrTest, BinaryReadInstrWithCondition) {
  Event::reset_id(7);
  uintptr_t condition_read_access = 0x03fa;

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  uintptr_t access = 0x03fa;
  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(access, condition_ptr));
  std::unique_ptr<ReadEvent<long>> event_ptr_b(new ReadEvent<long>(access + 1, condition_ptr));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));
  const BinaryReadInstr<ADD, int, long> instr(std::move(basic_read_instr_a) /* explicit move */,
    std::unique_ptr<ReadInstr<long>>(new BasicReadInstr<long>(std::move(event_ptr_b))) /* implicit move */ );

  const BasicReadInstr<int>& left_child = dynamic_cast<const BasicReadInstr<int>&>(instr.loperand_ref());
  const BasicReadInstr<long>& right_child = dynamic_cast<const BasicReadInstr<long>&>(instr.roperand_ref());
  EXPECT_EQ(8, left_child.event_ptr()->id());
  EXPECT_EQ(1, left_child.event_ptr()->addr().ptrs().size());

  EXPECT_EQ(9, right_child.event_ptr()->id());
  EXPECT_EQ(1, right_child.event_ptr()->addr().ptrs().size());
}

// The library forbids a binary operator to be constructed from distinct conditions.
TEST(InstrTest, BinaryReadInstrWithDifferentPathConditions) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";

  uintptr_t condition_read_access = 0x03fa;

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr_a(new ReadEvent<bool>(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr_a(
    new BasicReadInstr<bool>(std::move(condition_event_ptr_a)));

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr_b(new ReadEvent<bool>(condition_read_access));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr_b(
    new BasicReadInstr<bool>(std::move(condition_event_ptr_b)));

  uintptr_t access = 0x03fa;
  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(access, condition_ptr_a));
  std::unique_ptr<ReadEvent<long>> event_ptr_b(new ReadEvent<long>(access + 1, condition_ptr_b));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));
  std::unique_ptr<ReadInstr<long>> basic_read_instr_b(new BasicReadInstr<long>(std::move(event_ptr_b)));
  EXPECT_EXIT((BinaryReadInstr<ADD, int, long>(std::move(basic_read_instr_a),
    std::move(basic_read_instr_b))), ::testing::KilledBySignal(SIGABRT), "Assertion");
}

TEST(InstrTest, Filter) {
  Event::reset_id(7);

  uintptr_t access_a = 0x03fa;
  uintptr_t access_b = 0x03fb;
  std::unique_ptr<ReadEvent<int>> event_ptr_a(new ReadEvent<int>(access_a));
  std::unique_ptr<ReadEvent<long>> event_ptr_b(new ReadEvent<long>(access_b));
  std::unique_ptr<ReadInstr<int>> basic_read_instr_a(new BasicReadInstr<int>(std::move(event_ptr_a)));
  const BinaryReadInstr<ADD, int, long> instr(std::move(basic_read_instr_a) /* explicit move */,
    std::unique_ptr<ReadInstr<long>>(new BasicReadInstr<long>(std::move(event_ptr_b))) /* implicit move */ );

  std::forward_list<std::shared_ptr<Event>> event_ptrs;
  instr.filter(event_ptrs);

  const ReadEvent<long>& event_b = dynamic_cast<const ReadEvent<long>&>(*event_ptrs.front());
  EXPECT_EQ(8, event_b.id());

  event_ptrs.pop_front();

  const ReadEvent<int>& event_a = dynamic_cast<const ReadEvent<int>&>(*event_ptrs.front());
  EXPECT_EQ(7, event_a.id());

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

};

TEST(InstrTest, ReadInstrSwitch) {
  const LiteralReadInstr<int> literal_read_instr(5);

  uintptr_t access = 0x03fa;
  std::unique_ptr<ReadEvent<long>> event_ptr(new ReadEvent<long>(access));
  const BasicReadInstr<long> basic_read_instr(std::move(event_ptr));
  
  const ReadInstrPrinter printer;
  std::stringstream out;

  printer.switch_instr(literal_read_instr, out);
  printer.switch_instr(basic_read_instr, out);

  EXPECT_EQ("LiteralReadInstr\n", out.str());
}
