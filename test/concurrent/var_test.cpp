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
  EXPECT_FALSE(var.tag().is_bottom());
  EXPECT_EQ(2*7+1, var.direct_write_event_ref().event_id());
}

TEST(VarTest, LocalDeclVarInitScalar) {
  Threads::reset(7);
  Threads::begin_main_thread();

  DeclVar<char> var(false);

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ(0, instr.literal());
  EXPECT_TRUE(var.tag().is_bottom());
  EXPECT_EQ(2*7+1, var.direct_write_event_ref().event_id());
}

TEST(VarTest, LocalVarInitScalar) {
  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<char> var('Z');

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ('Z', instr.literal());
  EXPECT_TRUE(var.tag().is_bottom());
}

TEST(VarTest, LocalVarInitArray) {
  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<char[5]> var;

  EXPECT_TRUE(var[0].tag().is_bottom());
  EXPECT_TRUE(var[1].tag().is_bottom());
  EXPECT_TRUE(var[2].tag().is_bottom());
  EXPECT_TRUE(var[3].tag().is_bottom());
  EXPECT_TRUE(var[4].tag().is_bottom());
}

TEST(VarTest, SharedVarInitScalar) {
  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> var('Z');

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.direct_write_event_ref().instr_ref());
  EXPECT_EQ('Z', instr.literal());
  EXPECT_FALSE(var.tag().is_bottom());
}

TEST(VarTest, SharedVarInitArray) {
  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[5]> var;

  EXPECT_FALSE(var[0].tag().is_bottom());
  EXPECT_FALSE(var[1].tag().is_bottom());
  EXPECT_FALSE(var[2].tag().is_bottom());
  EXPECT_FALSE(var[3].tag().is_bottom());
  EXPECT_FALSE(var[4].tag().is_bottom());
}

TEST(VarTest, CopyLocalVar) {
  Threads::reset(7);
  Threads::begin_main_thread();

  LocalVar<char> var;
  EXPECT_EQ(2*7+1, var.direct_write_event_ref().event_id());

  LocalVar<char> other = var;
  EXPECT_EQ(2*8+1, other.direct_write_event_ref().event_id());

  // local variables can never be linked through RF, FR etc.
  EXPECT_TRUE(var.tag().is_bottom());
  EXPECT_TRUE(other.tag().is_bottom());
  EXPECT_EQ(var.tag(), other.tag());
}

TEST(VarTest, CopySharedVarToLocalVar) {
  Threads::reset(7);
  Threads::begin_main_thread();

  SharedVar<char> var;
  EXPECT_EQ(2*7+1, var.direct_write_event_ref().event_id());
  EXPECT_FALSE(var.tag().is_bottom());

  LocalVar<char> other = var;
  EXPECT_EQ(2*9+1, other.direct_write_event_ref().event_id());
  EXPECT_NE(var.tag(), other.tag());
  EXPECT_TRUE(other.tag().is_bottom());
}
