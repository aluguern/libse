#include <sstream>
#include "gtest/gtest.h"
#include "sequential-se.h"

using namespace se;

TEST(LoopTest, BoundedUnwindingPolicy) {
  const Value<bool> anything = any_bool("ANY");
  BoundedUnwindingPolicy policy(2u);
  EXPECT_TRUE(policy.unwind(anything));
  EXPECT_TRUE(policy.unwind(anything));

  EXPECT_FALSE(policy.unwind(anything));

  EXPECT_FALSE(policy.unwind(anything));
  EXPECT_FALSE(policy.unwind(anything));
  // ...
}

TEST(LoopTest, LoopWithBoundedUnwindingPolicy) {
  const Value<bool> anything = any_bool("ANY");
  std::unique_ptr<UnwindingPolicy> policy_ptr(new BoundedUnwindingPolicy(2u));
  Loop loop(std::move(policy_ptr));

  EXPECT_FALSE(policy_ptr);

  EXPECT_TRUE(loop.unwind(anything));
  EXPECT_TRUE(loop.unwind(anything));

  EXPECT_FALSE(loop.unwind(anything));

  EXPECT_FALSE(loop.unwind(anything));
  EXPECT_FALSE(loop.unwind(anything));
  // ...
}

TEST(LoopTest, LoopWithK) {
  const Value<bool> anything = any_bool("ANY");
  Loop loop(2u);

  EXPECT_TRUE(loop.unwind(anything));
  EXPECT_TRUE(loop.unwind(anything));

  EXPECT_FALSE(loop.unwind(anything));

  EXPECT_FALSE(loop.unwind(anything));
  EXPECT_FALSE(loop.unwind(anything));
  // ...
}

// k is less than the number of loop iterations induced by concrete values.
TEST(LoopTest, UnwindWithConcreteConditionLessThanK) {
  bool ok;
  Int i = 1;
  Loop loop(1u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 2;

  std::stringstream out_1x;
  out_1x << i.get_value().get_expr();
  EXPECT_EQ("3", out_1x.str());

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 4;

  std::stringstream out_2x;
  out_2x << i.get_value().get_expr();
  EXPECT_EQ("7", out_2x.str());
  
  // 3x
  ok = loop.unwind(i < 7);
  EXPECT_FALSE(ok);

  std::stringstream out;
  out << i.get_value().get_expr();
  EXPECT_EQ("7", out.str());
}

// k is equal to the number of loop iterations induced by concrete values.
TEST(LoopTest, UnwindWithConcreteConditionEqualK) {
  bool ok;
  Int i = 1;
  Loop loop(1u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 2;

  std::stringstream out_1x;
  out_1x << i.get_value().get_expr();
  EXPECT_EQ("3", out_1x.str());

  // 2x
  ok = loop.unwind(i < 3);
  EXPECT_FALSE(ok);

  // unwind(const Value<bool>&) call for 3x is undefined because ok == false.

  std::stringstream out;
  out << i.get_value().get_expr();
  EXPECT_EQ("3", out.str());
}

// k is greater than the number of loop iterations induced by concrete values.
TEST(LoopTest, UnwindWithConcreteConditionGreaterThanK) {
  bool ok;
  Int i = 1;
  Loop loop(5u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 2;

  std::stringstream out_1x;
  out_1x << i.get_value().get_expr();
  EXPECT_EQ("3", out_1x.str());

  // 2x
  ok = loop.unwind(i < 3);
  EXPECT_FALSE(ok);

  // unwind(const Value<bool>&) call for 3x is undefined because ok == false.

  std::stringstream out;
  out << i.get_value().get_expr();
  EXPECT_EQ("3", out.str());
}

// A concrete loop condition becomes symbolic.
TEST(LoopTest, UnwindWithConcreteAndSymbolicCondition) {
  bool ok;
  Int i = 1;
  Loop loop(1u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 2;

  std::stringstream out_1x;
  out_1x << i.get_value().get_expr();
  EXPECT_EQ("3", out_1x.str());

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  std::stringstream out_2x;
  out_2x << i.get_value().get_expr();
  EXPECT_EQ("(3+[A])", out_2x.str());

  // 3x
  ok = loop.unwind(i < 8);
  EXPECT_TRUE(ok);

  i = i + any_int("B");

  std::stringstream out_3x;
  out_3x << i.get_value().get_expr();
  EXPECT_EQ("((3+[A])+[B])", out_3x.str());

  // 4x
  ok = loop.unwind(i < 9);
  EXPECT_FALSE(ok);

  std::stringstream out;
  out << i.get_value().get_expr();
  EXPECT_EQ("(((3+[A])<8)?((3+[A])+[B]):(3+[A]))", out.str());
}

TEST(LoopTest, VersionAfterUnwind1xWithSingleVar) {
  bool ok;
  Int i = any_int("I");
  Loop loop(1u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + 1;

  EXPECT_EQ(VZERO + 1, i.get_version());

  // 2x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  // version increase due to join operation
  EXPECT_EQ(VZERO + 2, i.get_version());
}

TEST(LoopTest, Unwind1xWithSingleVar) {
  bool ok;
  Int i = any_int("I");
  Loop loop(1u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + 1;

  // 2x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream out;
  out << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+1):[I])", out.str());
}

TEST(LoopTest, Unwind2xWithSingleVar) {
  bool ok;
  Int i = any_int("I");
  Loop loop(2u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + 1;

  std::stringstream out_1x;
  out_1x << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_1x.str());

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 2;

  std::stringstream out_2x;
  out_2x << i.get_value().get_expr();
  EXPECT_EQ("([I]+3)", out_2x.str());

  // 3x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream join_out;
  join_out << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?([I]+3):([I]+1)):[I])", join_out.str());
}

TEST(LoopTest, Unwind3xWithSingleVar) {
  bool ok;
  Int i = any_int("I");
  Loop loop(3u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + 1;

  std::stringstream out_1x;
  out_1x << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_1x.str());

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 2;

  std::stringstream out_2x;
  out_2x << i.get_value().get_expr();
  EXPECT_EQ("([I]+3)", out_2x.str());
 
  // 3x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 3;

  std::stringstream out_3x;
  out_3x << i.get_value().get_expr();
  EXPECT_EQ("([I]+6)", out_3x.str());

  // 4x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream join_out;
  join_out << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?((([I]+3)<7)?([I]+6):([I]+3)):([I]+1)):[I])", join_out.str());
}

TEST(LoopTest, Unwind4xWithSingleVar) {
  bool ok;
  Int i = any_int("I");
  Loop loop(4u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + 1;

  std::stringstream out_1x;
  out_1x << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_1x.str());

  // 2x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + 2;

  std::stringstream out_2x;
  out_2x << i.get_value().get_expr();
  EXPECT_EQ("([I]+3)", out_2x.str());
 
  // 3x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + 3;

  std::stringstream out_3x;
  out_3x << i.get_value().get_expr();
  EXPECT_EQ("([I]+6)", out_3x.str());

  // 4x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + 4;

  std::stringstream out_4x;
  out_4x << i.get_value().get_expr();
  EXPECT_EQ("([I]+10)", out_4x.str());

  // 5x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream join_out;
  join_out << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+1)<5)?((([I]+3)<5)?((([I]+6)<5)?([I]+10):([I]+6)):([I]+3)):([I]+1)):[I])", join_out.str());
}

TEST(LoopTest, Unwind2xWithMultipleVars) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");
  Loop loop(2u);
  loop.track(i);
  loop.track(j);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + 1;
  j = j + 1;

  std::stringstream out_i_1x;
  out_i_1x << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_i_1x.str());

  std::stringstream out_j_1x;
  out_j_1x << j.get_value().get_expr();
  EXPECT_EQ("([J]+1)", out_j_1x.str());

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 2;
  j = j + 2;

  std::stringstream out_i_2x;
  out_i_2x << i.get_value().get_expr();
  EXPECT_EQ("([I]+3)", out_i_2x.str());

  std::stringstream out_j_2x;
  out_j_2x << j.get_value().get_expr();
  EXPECT_EQ("([J]+3)", out_j_2x.str());

  // 3x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream out_i_join;
  out_i_join << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?([I]+3):([I]+1)):[I])", out_i_join.str());

  std::stringstream out_j_join;
  out_j_join << j.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?([J]+3):([J]+1)):[J])", out_j_join.str());
}

TEST(LoopTest, Unwind3xWithMultipleVars) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");
  Loop loop(3u);
  loop.track(i);
  loop.track(j);

  // 1x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 1;
  j = j + 2;

  std::stringstream out_i_1x;
  out_i_1x << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_i_1x.str());

  std::stringstream out_j_1x;
  out_j_1x << j.get_value().get_expr();
  EXPECT_EQ("([J]+2)", out_j_1x.str());

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 3;
  j = j + 4;

  std::stringstream out_i_2x;
  out_i_2x << i.get_value().get_expr();
  EXPECT_EQ("([I]+4)", out_i_2x.str());

  std::stringstream out_j_2x;
  out_j_2x << j.get_value().get_expr();
  EXPECT_EQ("([J]+6)", out_j_2x.str());

  // 3x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + 5;
  j = j + 6;

  std::stringstream out_i_3x;
  out_i_3x << i.get_value().get_expr();
  EXPECT_EQ("([I]+9)", out_i_3x.str());

  std::stringstream out_j_3x;
  out_j_3x << j.get_value().get_expr();
  EXPECT_EQ("([J]+12)", out_j_3x.str());

  // 4x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream out_i_join;
  out_i_join << i.get_value().get_expr();
  EXPECT_EQ("(([I]<7)?((([I]+1)<7)?((([I]+4)<7)?([I]+9):([I]+4)):([I]+1)):[I])", out_i_join.str());

  std::stringstream out_j_join;
  out_j_join << j.get_value().get_expr();
  EXPECT_EQ("(([I]<7)?((([I]+1)<7)?((([I]+4)<7)?([J]+12):([J]+6)):([J]+2)):[J])", out_j_join.str());
}

TEST(LoopTest, MultipleTrack) {
  bool ok;
  Int i = any_int("I");
  Loop loop(2u);

  loop.track(i);
  EXPECT_NO_THROW(loop.track(i));
}

TEST(LoopTest, VersionAfterUnwind1xWithSingleVarWithAny) {
  bool ok;
  Int i = any_int("I");
  Loop loop(1u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  EXPECT_EQ(VZERO + 1, i.get_version());

  // 2x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  // version increase due to join operation
  EXPECT_EQ(VZERO + 2, i.get_version());
}

TEST(LoopTest, Unwind1xWithSingleVarWithAny) {
  bool ok;
  Int i = any_int("I");
  Loop loop(1u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  // 2x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream out;
  out << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+[A]):[I])", out.str());
}

TEST(LoopTest, Unwind2xWithSingleVarWithAny) {
  bool ok;
  Int i = any_int("I");
  Loop loop(2u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  std::stringstream out_1x;
  out_1x << i.get_value().get_expr();
  EXPECT_EQ("([I]+[A])", out_1x.str());

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + any_int("B");

  std::stringstream out_2x;
  out_2x << i.get_value().get_expr();
  EXPECT_EQ("(([I]+[A])+[B])", out_2x.str());

  // 3x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream join_out;
  join_out << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+[A])<7)?(([I]+[A])+[B]):([I]+[A])):[I])", join_out.str());
}

TEST(LoopTest, Unwind3xWithSingleVarWithAny) {
  bool ok;
  Int i = any_int("I");
  Loop loop(3u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  std::stringstream out_1x;
  out_1x << i.get_value().get_expr();
  EXPECT_EQ("([I]+[A])", out_1x.str());

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + any_int("B");

  std::stringstream out_2x;
  out_2x << i.get_value().get_expr();
  EXPECT_EQ("(([I]+[A])+[B])", out_2x.str());
 
  // 3x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + any_int("C");

  std::stringstream out_3x;
  out_3x << i.get_value().get_expr();
  EXPECT_EQ("((([I]+[A])+[B])+[C])", out_3x.str());

  // 4x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream join_out;
  join_out << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+[A])<7)?(((([I]+[A])+[B])<7)?((([I]+[A])+[B])+[C]):(([I]+[A])+[B])):([I]+[A])):[I])", join_out.str());
}

TEST(LoopTest, Unwind4xWithSingleVarWithAny) {
  bool ok;
  Int i = any_int("I");
  Loop loop(4u);
  loop.track(i);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  std::stringstream out_1x;
  out_1x << i.get_value().get_expr();
  EXPECT_EQ("([I]+[A])", out_1x.str());

  // 2x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + any_int("B");

  std::stringstream out_2x;
  out_2x << i.get_value().get_expr();
  EXPECT_EQ("(([I]+[A])+[B])", out_2x.str());
 
  // 3x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + any_int("C");

  std::stringstream out_3x;
  out_3x << i.get_value().get_expr();
  EXPECT_EQ("((([I]+[A])+[B])+[C])", out_3x.str());

  // 4x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + any_int("D");

  std::stringstream out_4x;
  out_4x << i.get_value().get_expr();
  EXPECT_EQ("(((([I]+[A])+[B])+[C])+[D])", out_4x.str());

  // 5x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream join_out;
  join_out << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+[A])<5)?(((([I]+[A])+[B])<5)?((((([I]+[A])+[B])+[C])<5)?(((([I]+[A])+[B])+[C])+[D]):((([I]+[A])+[B])+[C])):(([I]+[A])+[B])):([I]+[A])):[I])", join_out.str());
}

TEST(LoopTest, Unwind2xWithMultipleVarsWithAny) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");
  Loop loop(2u);
  loop.track(i);
  loop.track(j);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  i = i + any_int("A");
  j = j + any_int("A");

  std::stringstream out_i_1x;
  out_i_1x << i.get_value().get_expr();
  EXPECT_EQ("([I]+[A])", out_i_1x.str());

  std::stringstream out_j_1x;
  out_j_1x << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_j_1x.str());

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + any_int("B");
  j = j + any_int("B");

  std::stringstream out_i_2x;
  out_i_2x << i.get_value().get_expr();
  EXPECT_EQ("(([I]+[A])+[B])", out_i_2x.str());

  std::stringstream out_j_2x;
  out_j_2x << j.get_value().get_expr();
  EXPECT_EQ("(([J]+[A])+[B])", out_j_2x.str());

  // 3x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream out_i_join;
  out_i_join << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+[A])<7)?(([I]+[A])+[B]):([I]+[A])):[I])", out_i_join.str());

  std::stringstream out_j_join;
  out_j_join << j.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+[A])<7)?(([J]+[A])+[B]):([J]+[A])):[J])", out_j_join.str());
}

TEST(LoopTest, Unwind3xWithMultipleVarsWithAny) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");
  Loop loop(3u);
  loop.track(i);
  loop.track(j);

  // 1x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + any_int("A");
  j = j + any_int("B");

  std::stringstream out_i_1x;
  out_i_1x << i.get_value().get_expr();
  EXPECT_EQ("([I]+[A])", out_i_1x.str());

  std::stringstream out_j_1x;
  out_j_1x << j.get_value().get_expr();
  EXPECT_EQ("([J]+[B])", out_j_1x.str());

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + any_int("C");
  j = j + any_int("D");

  std::stringstream out_i_2x;
  out_i_2x << i.get_value().get_expr();
  EXPECT_EQ("(([I]+[A])+[C])", out_i_2x.str());

  std::stringstream out_j_2x;
  out_j_2x << j.get_value().get_expr();
  EXPECT_EQ("(([J]+[B])+[D])", out_j_2x.str());

  // 3x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  i = i + any_int("E");
  j = j + any_int("F");

  std::stringstream out_i_3x;
  out_i_3x << i.get_value().get_expr();
  EXPECT_EQ("((([I]+[A])+[C])+[E])", out_i_3x.str());

  std::stringstream out_j_3x;
  out_j_3x << j.get_value().get_expr();
  EXPECT_EQ("((([J]+[B])+[D])+[F])", out_j_3x.str());

  // 4x
  ok = loop.unwind(any_bool("ANY"));
  EXPECT_FALSE(ok);

  std::stringstream out_i_join;
  out_i_join << i.get_value().get_expr();
  EXPECT_EQ("(([I]<7)?((([I]+[A])<7)?(((([I]+[A])+[C])<7)?((([I]+[A])+[C])+[E]):(([I]+[A])+[C])):([I]+[A])):[I])", out_i_join.str());

  std::stringstream out_j_join;
  out_j_join << j.get_value().get_expr();
  EXPECT_EQ("(([I]<7)?((([I]+[A])<7)?(((([I]+[A])+[C])<7)?((([J]+[B])+[D])+[F]):(([J]+[B])+[D])):([J]+[B])):[J])", out_j_join.str());
}

