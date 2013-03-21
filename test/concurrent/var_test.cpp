#include "gtest/gtest.h"

#include "concurrent/var.h"

using namespace se;

TEST(VarTest, SharedDeclVarInitScalar) {
  Event::reset_id(7);

  DeclVar<char> var(true);

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ(0, instr.literal());
  EXPECT_TRUE(var.addr().is_shared());
}

TEST(VarTest, LocalDeclVarInitScalar) {
  Event::reset_id(7);

  DeclVar<char> var(false);

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ(0, instr.literal());
  EXPECT_FALSE(var.addr().is_shared());
}
