#include "gtest/gtest.h"

#include "concurrent/var.h"

using namespace se;

TEST(VarTest, DeclVarInitScalar) {
  Event::reset_id(7);

  DeclVar<char> var;

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.write_event_ref().instr_ref());
  EXPECT_EQ(0, instr.literal());
}

TEST(VarTest, DeclVarInitArray) {
  Event::reset_id(7);

  DeclVar<char[5]> var;

  const LiteralReadInstr<char[5]>& instr = dynamic_cast<const LiteralReadInstr<char[5]>&>(var.write_event_ref().instr_ref());
  EXPECT_EQ(0, instr.literal()[0]);
  EXPECT_EQ(0, instr.literal()[1]);
  EXPECT_EQ(0, instr.literal()[2]);
  EXPECT_EQ(0, instr.literal()[3]);
  EXPECT_EQ(0, instr.literal()[4]);
}
