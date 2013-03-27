#include "gtest/gtest.h"

#include "concurrent/var.h"
#include "concurrent/recorder.h"

using namespace se;

TEST(VarTest, SharedDeclVarInitScalar) {
  init_recorder();
  Event::reset_id(7);

  DeclVar<char> var(true);

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ(0, instr.literal());
  EXPECT_TRUE(var.addr().is_shared());
  EXPECT_EQ(2*7+1, var.direct_write_event_ref().event_id());
}

TEST(VarTest, LocalDeclVarInitScalar) {
  init_recorder();
  Event::reset_id(7);

  DeclVar<char> var(false);

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ(0, instr.literal());
  EXPECT_FALSE(var.addr().is_shared());
  EXPECT_EQ(2*7+1, var.direct_write_event_ref().event_id());
}

TEST(VarTest, LocalVarInitScalar) {
  init_recorder();

  LocalVar<char> var('Z');

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ('Z', instr.literal());
  EXPECT_FALSE(var.addr().is_shared());
}

TEST(VarTest, SharedVarInitScalar) {
  init_recorder();

  SharedVar<char> var('Z');

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ('Z', instr.literal());
  EXPECT_TRUE(var.addr().is_shared());
}
