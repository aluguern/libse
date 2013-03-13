#include "gtest/gtest.h"

#include "concurrent/var.h"

using namespace se;

TEST(VarTest, DeclVarInitScalar) {
  Event::reset_id(7);

  DeclVar<char> var;

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.write_event_ref().instr_ref());
  EXPECT_EQ(0, instr.literal());
}
