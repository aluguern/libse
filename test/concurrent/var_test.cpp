#include "gtest/gtest.h"

#include "concurrent/var.h"
#include "concurrent/thread.h"

using namespace se;

TEST(VarTest, SharedDeclVarInitScalar) {
  Threads::reset(7);
  Threads::begin_main_thread();

  DeclVar<char> var(true);

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ(0, instr.literal());
  EXPECT_TRUE(var.addr().is_shared());
  EXPECT_EQ(2*7+1, var.direct_write_event_ref().event_id());
}

TEST(VarTest, LocalDeclVarInitScalar) {
  Threads::reset(7);
  Threads::begin_main_thread();

  DeclVar<char> var(false);

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ(0, instr.literal());
  EXPECT_FALSE(var.addr().is_shared());
  EXPECT_EQ(2*7+1, var.direct_write_event_ref().event_id());
}

TEST(VarTest, LocalVarInitScalar) {
  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<char> var('Z');

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ('Z', instr.literal());
  EXPECT_FALSE(var.addr().is_shared());
}

TEST(VarTest, LocalVarInitArray) {
  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<char[5]> var;

  EXPECT_FALSE(var[0].addr().is_shared());
  EXPECT_FALSE(var[1].addr().is_shared());
  EXPECT_FALSE(var[2].addr().is_shared());
  EXPECT_FALSE(var[3].addr().is_shared());
  EXPECT_FALSE(var[4].addr().is_shared());
}

TEST(VarTest, SharedVarInitScalar) {
  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> var('Z');

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ('Z', instr.literal());
  EXPECT_TRUE(var.addr().is_shared());
}

TEST(VarTest, SharedVarInitArray) {
  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[5]> var;

  EXPECT_TRUE(var[0].addr().is_shared());
  EXPECT_TRUE(var[1].addr().is_shared());
  EXPECT_TRUE(var[2].addr().is_shared());
  EXPECT_TRUE(var[3].addr().is_shared());
  EXPECT_TRUE(var[4].addr().is_shared());
}

TEST(VarTest, CopyLocalVar) {
  Threads::reset(7);
  Threads::begin_main_thread();

  LocalVar<char> var;
  EXPECT_EQ(2*7+1, var.direct_write_event_ref().event_id());

  LocalVar<char> other = var;
  EXPECT_EQ(2*8+1, other.direct_write_event_ref().event_id());
  EXPECT_NE(var.addr(), other.addr());
}
