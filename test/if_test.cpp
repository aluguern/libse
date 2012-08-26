#include <sstream>
#include "gtest/gtest.h"
#include "sequential-se.h"

using namespace se;

TEST(IfTest, ThenElseWithConcreteTrueCondition) {
  bool ok;
  Int i = 2;
  If branch(i < 5);
  branch.track(i);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  ok = branch.begin_else();
  EXPECT_FALSE(ok);

  branch.end();

  std::stringstream out_end;
  out_end << i.get_value().get_expr();
  EXPECT_EQ("(2+[A])", out_end.str());
}

TEST(IfTest, ThenElseWithConcreteFalseCondition) {
  bool ok;
  Int i = 7;
  If branch(i < 5);
  branch.track(i);

  ok = branch.begin_then();
  EXPECT_FALSE(ok);

  ok = branch.begin_else();
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  branch.end();

  std::stringstream out_end;
  out_end << i.get_value().get_expr();
  EXPECT_EQ("(7+[A])", out_end.str());
}

TEST(IfTest, VersionAfterThenWithSingleVar) {
  bool ok;
  Int i = any_int("I");
  If branch(i < 5);
  branch.track(i);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;

  EXPECT_EQ(VZERO + 1, i.get_version());

  branch.end();

  // version increase due to join operation
  EXPECT_EQ(VZERO + 2, i.get_version());
}

TEST(IfTest, VersionAfterThenWithSingleAnyVar) {
  bool ok;
  Int i = any_int("I");
  If branch(i < 5);
  branch.track(i);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  EXPECT_EQ(VZERO + 1, i.get_version());

  branch.end();

  // version increase due to join operation
  EXPECT_EQ(VZERO + 2, i.get_version());
}

TEST(IfTest, ThenWithSingleVar) {
  bool ok;
  Int i = any_int("I");
  If branch(i < 5);
  branch.track(i);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;

  std::stringstream out_then;
  out_then << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_then.str());
  
  branch.end();
  
  std::stringstream out_end;
  out_end << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+1):[I])", out_end.str());
}

TEST(IfTest, ThenWithSingleAnyVar) {
  bool ok;
  Int i = any_int("I");
  If branch(i < 5);
  branch.track(i);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  branch.end();

  std::stringstream out_end;
  out_end << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+[A]):[I])", out_end.str());
}

TEST(IfTest, ThenWithMultipleVars) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");
  If branch(i < 5);
  branch.track(i);
  branch.track(j);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;
  j = j + 2;

  std::stringstream out_then_i;
  out_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_then_i.str());

  std::stringstream out_then_j;
  out_then_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+2)", out_then_j.str());

  branch.end();

  std::stringstream out_end_i;
  out_end_i << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+1):[I])", out_end_i.str());

  std::stringstream out_end_j;
  out_end_j << j.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([J]+2):[J])", out_end_j.str());
}

TEST(IfTest, ThenWithMultipleAnyVars) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");
  If branch(i < 5);
  branch.track(i);
  branch.track(j);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + any_int("A");
  j = j + any_int("A");

  std::stringstream out_then_i;
  out_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+[A])", out_then_i.str());

  std::stringstream out_then_j;
  out_then_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_then_j.str());

  branch.end();

  std::stringstream out_end_i;
  out_end_i << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+[A]):[I])", out_end_i.str());

  std::stringstream out_end_j;
  out_end_j << j.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([J]+[A]):[J])", out_end_j.str());
}

TEST(IfTest, VersionAfterThenWithExtraVar) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");

  If branch(i < 5);
  branch.track(i);
  branch.track(j);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;

  EXPECT_EQ(VZERO + 1, i.get_version());
  EXPECT_EQ(VZERO, j.get_version());

  branch.end();

  // version increase due to join operation
  EXPECT_EQ(VZERO + 2, i.get_version());
  EXPECT_EQ(VZERO, j.get_version());
}

TEST(IfTest, ThenWithExtraVar) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");

  If branch(i < 5);
  branch.track(i);
  branch.track(j);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;

  std::stringstream out_then_i;
  out_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_then_i.str());

  std::stringstream out_then_j;
  out_then_j << j.get_value().get_expr();
  EXPECT_EQ("[J]", out_then_j.str());

  branch.end();

  std::stringstream out_end_i;
  out_end_i << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+1):[I])", out_end_i.str());

  std::stringstream out_end_j;
  out_end_j << j.get_value().get_expr();
  EXPECT_EQ("[J]", out_end_j.str());
}

TEST(IfTest, NestedThenWithExtraAndAnyVars) {
  bool ok;

  // only modified in outer scope
  Int i = any_int("I");
  Int j = any_int("J");

  // only modified in inner scope
  Int k = any_int("K");

  // modified in both outer and inner scope
  Int x = any_int("X");

  // never modified in either outer or inner scope
  Int c = any_int("C");

  If outer_if(i < 5);
  outer_if.track(i);
  outer_if.track(j);
  outer_if.track(k);
  outer_if.track(x);
  outer_if.track(c);

  ok = outer_if.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;
  j = j + any_int("A");
  x = x + any_int("B");

  std::stringstream out_outer_then_i;
  out_outer_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_outer_then_i.str());

  std::stringstream out_outer_then_j;
  out_outer_then_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_outer_then_j.str());

  std::stringstream out_outer_then_k;
  out_outer_then_k << k.get_value().get_expr();
  EXPECT_EQ("[K]", out_outer_then_k.str());

  std::stringstream out_outer_then_x;
  out_outer_then_x << x.get_value().get_expr();
  EXPECT_EQ("([X]+[B])", out_outer_then_x.str());

  std::stringstream out_outer_then_c;
  out_outer_then_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_outer_then_c.str());

  If inner_if(i < 7);
  inner_if.track(i);
  inner_if.track(j);
  inner_if.track(k);
  inner_if.track(x);
  inner_if.track(c);

  ok = inner_if.begin_then();
  EXPECT_TRUE(ok);

  x = x + 2;
  k = k + 3;

  std::stringstream out_inner_then_i;
  out_inner_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_inner_then_i.str());

  std::stringstream out_inner_then_j;
  out_inner_then_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_inner_then_j.str());

  std::stringstream out_inner_then_k;
  out_inner_then_k << k.get_value().get_expr();
  EXPECT_EQ("([K]+3)", out_inner_then_k.str());

  std::stringstream out_inner_then_x;
  out_inner_then_x << x.get_value().get_expr();
  // TODO: Support associative operations over symbolic variables.
  EXPECT_EQ("(([X]+[B])+2)", out_inner_then_x.str());

  std::stringstream out_inner_then_c;
  out_inner_then_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_inner_then_c.str());

  inner_if.end();

  std::stringstream out_inner_end_i;
  out_inner_end_i << i.get_value().get_expr();
  // TODO: Optimize NaryExpr partial expressions.
  EXPECT_EQ("((([I]+1)<7)?([I]+1):([I]+1))", out_inner_end_i.str());

  std::stringstream out_inner_end_j;
  out_inner_end_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_inner_end_j.str());

  std::stringstream out_inner_end_k;
  out_inner_end_k << k.get_value().get_expr();
  EXPECT_EQ("((([I]+1)<7)?([K]+3):[K])", out_inner_end_k.str());

  std::stringstream out_inner_end_x;
  out_inner_end_x << x.get_value().get_expr();
  // TODO: Support associative operations over symbolic variables.
  EXPECT_EQ("((([I]+1)<7)?(([X]+[B])+2):([X]+[B]))", out_inner_end_x.str());

  std::stringstream out_inner_end_c;
  out_inner_end_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_inner_end_c.str());

  outer_if.end();

  std::stringstream out_outer_end_i;
  out_outer_end_i << i.get_value().get_expr();
  // TODO: Optimize NaryExpr partial expressions.
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?([I]+1):([I]+1)):[I])", out_outer_end_i.str());

  std::stringstream out_outer_end_j;
  out_outer_end_j << j.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([J]+[A]):[J])", out_outer_end_j.str());

  std::stringstream out_outer_end_k;
  out_outer_end_k << k.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?([K]+3):[K]):[K])", out_outer_end_k.str());

  std::stringstream out_outer_end_x;
  out_outer_end_x << x.get_value().get_expr();
  // TODO: Support associative operations over symbolic variables.
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?(([X]+[B])+2):([X]+[B])):[X])", out_outer_end_x.str());

  std::stringstream out_outer_end_c;
  out_outer_end_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_outer_end_c.str());
}

TEST(IfTest, MultipleTrack) {
  bool ok;
  Int i = any_int("I");
  If branch(i < 5);

  branch.track(i);
  EXPECT_NO_THROW(branch.track(i));
}

TEST(IfTest, VersionAfterThenElseWithSingleVar) {
  bool ok;
  Int i = any_int("I");
  If branch(i < 5);
  branch.track(i);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;

  EXPECT_EQ(VZERO + 1, i.get_version());

  ok = branch.begin_else();
  EXPECT_TRUE(ok);

  EXPECT_EQ(VZERO + 2, i.get_version());

  i = i + 2;

  EXPECT_EQ(VZERO + 3, i.get_version());

  branch.end();

  // version increase due to join operation
  EXPECT_EQ(VZERO + 4, i.get_version());
}

TEST(IfTest, VersionAfterThenElseWithSingleAnyVar) {
  bool ok;
  Int i = any_int("I");
  If branch(i < 5);
  branch.track(i);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  EXPECT_EQ(VZERO + 1, i.get_version());

  ok = branch.begin_else();
  EXPECT_TRUE(ok);

  EXPECT_EQ(VZERO + 2, i.get_version());

  i = i + any_int("A");

  EXPECT_EQ(VZERO + 3, i.get_version());

  branch.end();

  // version increase due to join operation
  EXPECT_EQ(VZERO + 4, i.get_version());
}

TEST(IfTest, ThenElseWithSingleVar) {
  bool ok;
  Int i = any_int("I");
  If branch(i < 5);
  branch.track(i);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;

  std::stringstream out_then;
  out_then << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_then.str());

  ok = branch.begin_else();
  EXPECT_TRUE(ok);

  i = i + 2;

  std::stringstream out_else;
  out_else << i.get_value().get_expr();
  EXPECT_EQ("([I]+2)", out_else.str());

  branch.end();
  
  std::stringstream out_end;
  out_end << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+1):([I]+2))", out_end.str());
}

TEST(IfTest, ThenElseWithDifferentVar) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");
  If branch(i < 5);
  branch.track(i);
  branch.track(j);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;

  std::stringstream out_then_i;
  out_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_then_i.str());

  std::stringstream out_then_j;
  out_then_j << j.get_value().get_expr();
  EXPECT_EQ("[J]", out_then_j.str());

  ok = branch.begin_else();
  EXPECT_TRUE(ok);

  j = j + 2;

  std::stringstream out_else_i;
  out_else_i << i.get_value().get_expr();
  EXPECT_EQ("[I]", out_else_i.str());

  std::stringstream out_else_j;
  out_else_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+2)", out_else_j.str());

  branch.end();
  
  std::stringstream out_end_i;
  out_end_i << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+1):[I])", out_end_i.str());

  std::stringstream out_end_j;
  out_end_j << j.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?[J]:([J]+2))", out_end_j.str());
}

TEST(IfTest, ThenElseWithSingleAnyVar) {
  bool ok;
  Int i = any_int("I");
  If branch(i < 5);
  branch.track(i);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + any_int("A");

  ok = branch.begin_else();
  EXPECT_TRUE(ok);

  i = i + any_int("B");

  branch.end();

  std::stringstream out_end;
  out_end << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+[A]):([I]+[B]))", out_end.str());
}

TEST(IfTest, ThenElseWithMultipleVars) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");
  If branch(i < 5);
  branch.track(i);
  branch.track(j);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;
  j = j + 2;

  std::stringstream out_then_i;
  out_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_then_i.str());

  std::stringstream out_then_j;
  out_then_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+2)", out_then_j.str());

  ok = branch.begin_else();
  EXPECT_TRUE(ok);

  i = i + 3;
  j = j + 4;

  std::stringstream out_else_i;
  out_else_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+3)", out_else_i.str());

  std::stringstream out_else_j;
  out_else_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+4)", out_else_j.str());

  branch.end();

  std::stringstream out_end_i;
  out_end_i << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+1):([I]+3))", out_end_i.str());

  std::stringstream out_end_j;
  out_end_j << j.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([J]+2):([J]+4))", out_end_j.str());
}

TEST(IfTest, ThenElseWithMultipleAnyVars) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");
  If branch(i < 5);
  branch.track(i);
  branch.track(j);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + any_int("A");
  j = j + any_int("A");

  std::stringstream out_then_i;
  out_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+[A])", out_then_i.str());

  std::stringstream out_then_j;
  out_then_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_then_j.str());

  ok = branch.begin_else();
  EXPECT_TRUE(ok);

  i = i + any_int("C");
  j = j + any_int("D");

  std::stringstream out_else_i;
  out_else_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+[C])", out_else_i.str());

  std::stringstream out_else_j;
  out_else_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[D])", out_else_j.str());

  branch.end();

  std::stringstream out_end_i;
  out_end_i << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+[A]):([I]+[C]))", out_end_i.str());

  std::stringstream out_end_j;
  out_end_j << j.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([J]+[A]):([J]+[D]))", out_end_j.str());
}

TEST(IfTest, VersionAfterThenElseWithExtraVar) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");

  If branch(i < 5);
  branch.track(i);
  branch.track(j);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;

  EXPECT_EQ(VZERO + 1, i.get_version());

  EXPECT_EQ(VZERO, j.get_version());

  ok = branch.begin_else();
  EXPECT_TRUE(ok);

  EXPECT_EQ(VZERO + 2, i.get_version());

  // Even though j is unmodified, its expression is reset. That's OK.
  EXPECT_EQ(VZERO + 1, j.get_version());

  i = i + 2;

  EXPECT_EQ(VZERO + 3, i.get_version());
  EXPECT_EQ(VZERO + 1, j.get_version());

  branch.end();

  // version increase due to join operation
  EXPECT_EQ(VZERO + 4, i.get_version());

  // Even though j was not modified, its expression has been reset. That's OK.
  EXPECT_EQ(VZERO + 1, j.get_version());
}

TEST(IfTest, ThenElseWithExtraVar) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");

  If branch(i < 5);
  branch.track(i);
  branch.track(j);

  ok = branch.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;

  std::stringstream out_then_i;
  out_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_then_i.str());

  std::stringstream out_then_j;
  out_then_j << j.get_value().get_expr();
  EXPECT_EQ("[J]", out_then_j.str());

  ok = branch.begin_else();
  EXPECT_TRUE(ok);

  i = i + 2;

  std::stringstream out_else_i;
  out_else_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+2)", out_else_i.str());

  std::stringstream out_else_j;
  out_else_j << j.get_value().get_expr();
  EXPECT_EQ("[J]", out_else_j.str());
  
  branch.end();

  std::stringstream out_end_i;
  out_end_i << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([I]+1):([I]+2))", out_end_i.str());

  std::stringstream out_end_j;
  out_end_j << j.get_value().get_expr();
  EXPECT_EQ("[J]", out_end_j.str());
}

TEST(IfTest, ThenNestedThenElseWithExtraAndAnyVars) {
  bool ok;

  // only modified in outer scope
  Int i = any_int("I");
  Int j = any_int("J");

  // only modified in inner scope
  Int k = any_int("K");

  // modified in both outer and inner scope
  Int x = any_int("X");

  // never modified in either outer or inner scope
  Int c = any_int("C");

  If outer_if(i < 5);
  outer_if.track(i);
  outer_if.track(j);
  outer_if.track(k);
  outer_if.track(x);
  outer_if.track(c);

  ok = outer_if.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;
  j = j + any_int("A");
  x = x + any_int("B");

  std::stringstream out_outer_then_i;
  out_outer_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_outer_then_i.str());

  std::stringstream out_outer_then_j;
  out_outer_then_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_outer_then_j.str());

  std::stringstream out_outer_then_k;
  out_outer_then_k << k.get_value().get_expr();
  EXPECT_EQ("[K]", out_outer_then_k.str());

  std::stringstream out_outer_then_x;
  out_outer_then_x << x.get_value().get_expr();
  EXPECT_EQ("([X]+[B])", out_outer_then_x.str());

  std::stringstream out_outer_then_c;
  out_outer_then_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_outer_then_c.str());

  If inner_if(i < 7);
  inner_if.track(i);
  inner_if.track(j);
  inner_if.track(k);
  inner_if.track(x);
  inner_if.track(c);

  ok = inner_if.begin_then();
  EXPECT_TRUE(ok);

  x = x + 2;
  k = k + 3;

  std::stringstream out_inner_then_i;
  out_inner_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_inner_then_i.str());

  std::stringstream out_inner_then_j;
  out_inner_then_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_inner_then_j.str());

  std::stringstream out_inner_then_k;
  out_inner_then_k << k.get_value().get_expr();
  EXPECT_EQ("([K]+3)", out_inner_then_k.str());

  std::stringstream out_inner_then_x;
  out_inner_then_x << x.get_value().get_expr();
  // TODO: Support associative operations over symbolic variables.
  EXPECT_EQ("(([X]+[B])+2)", out_inner_then_x.str());

  std::stringstream out_inner_then_c;
  out_inner_then_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_inner_then_c.str());

  ok = inner_if.begin_else();
  EXPECT_TRUE(ok);

  x = x + 4;
  k = k + 5;

  std::stringstream out_inner_else_i;
  out_inner_else_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_inner_else_i.str());

  std::stringstream out_inner_else_j;
  out_inner_else_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_inner_else_j.str());

  std::stringstream out_inner_else_k;
  out_inner_else_k << k.get_value().get_expr();
  EXPECT_EQ("([K]+5)", out_inner_else_k.str());

  std::stringstream out_inner_else_x;
  out_inner_else_x << x.get_value().get_expr();
  // TODO: Support associative operations over symbolic variables.
  EXPECT_EQ("(([X]+[B])+4)", out_inner_else_x.str());

  std::stringstream out_inner_else_c;
  out_inner_else_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_inner_else_c.str());

  inner_if.end();

  std::stringstream out_inner_end_i;
  out_inner_end_i << i.get_value().get_expr();
  // TODO: Optimize NaryExpr partial expressions.
  EXPECT_EQ("((([I]+1)<7)?([I]+1):([I]+1))", out_inner_end_i.str());

  std::stringstream out_inner_end_j;
  out_inner_end_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_inner_end_j.str());

  std::stringstream out_inner_end_k;
  out_inner_end_k << k.get_value().get_expr();
  EXPECT_EQ("((([I]+1)<7)?([K]+3):([K]+5))", out_inner_end_k.str());

  std::stringstream out_inner_end_x;
  out_inner_end_x << x.get_value().get_expr();
  // TODO: Support associative operations over symbolic variables.
  EXPECT_EQ("((([I]+1)<7)?(([X]+[B])+2):(([X]+[B])+4))", out_inner_end_x.str());

  std::stringstream out_inner_end_c;
  out_inner_end_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_inner_end_c.str());

  outer_if.end();

  std::stringstream out_outer_end_i;
  out_outer_end_i << i.get_value().get_expr();
  // TODO: Optimize NaryExpr partial expressions.
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?([I]+1):([I]+1)):[I])", out_outer_end_i.str());

  std::stringstream out_outer_end_j;
  out_outer_end_j << j.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([J]+[A]):[J])", out_outer_end_j.str());

  std::stringstream out_outer_end_k;
  out_outer_end_k << k.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?([K]+3):([K]+5)):[K])", out_outer_end_k.str());

  std::stringstream out_outer_end_x;
  out_outer_end_x << x.get_value().get_expr();
  // TODO: Support associative operations over symbolic variables.
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?(([X]+[B])+2):(([X]+[B])+4)):[X])", out_outer_end_x.str());

  std::stringstream out_outer_end_c;
  out_outer_end_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_outer_end_c.str());
}

TEST(IfTest, ThenElseNestedThenWithExtraAndAnyVars) {
  bool ok;

  // only modified in outer scope
  Int i = any_int("I");
  Int j = any_int("J");

  // only modified in inner scope
  Int k = any_int("K");

  // modified in both outer and inner scope
  Int x = any_int("X");

  // never modified in either outer or inner scope
  Int c = any_int("C");

  If outer_if(i < 5);
  outer_if.track(i);
  outer_if.track(j);
  outer_if.track(k);
  outer_if.track(x);
  outer_if.track(c);

  ok = outer_if.begin_then();
  EXPECT_TRUE(ok);

  i = i + 1;
  j = j + any_int("A");
  x = x + any_int("B");

  std::stringstream out_outer_then_i;
  out_outer_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_outer_then_i.str());

  std::stringstream out_outer_then_j;
  out_outer_then_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_outer_then_j.str());

  std::stringstream out_outer_then_k;
  out_outer_then_k << k.get_value().get_expr();
  EXPECT_EQ("[K]", out_outer_then_k.str());

  std::stringstream out_outer_then_x;
  out_outer_then_x << x.get_value().get_expr();
  EXPECT_EQ("([X]+[B])", out_outer_then_x.str());

  std::stringstream out_outer_then_c;
  out_outer_then_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_outer_then_c.str());

  If inner_if(i < 7);
  inner_if.track(i);
  inner_if.track(j);
  inner_if.track(k);
  inner_if.track(x);
  inner_if.track(c);

  ok = inner_if.begin_then();
  EXPECT_TRUE(ok);

  x = x + 2;
  k = k + 3;

  std::stringstream out_inner_then_i;
  out_inner_then_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+1)", out_inner_then_i.str());

  std::stringstream out_inner_then_j;
  out_inner_then_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_inner_then_j.str());

  std::stringstream out_inner_then_k;
  out_inner_then_k << k.get_value().get_expr();
  EXPECT_EQ("([K]+3)", out_inner_then_k.str());

  std::stringstream out_inner_then_x;
  out_inner_then_x << x.get_value().get_expr();
  // TODO: Support associative operations over symbolic variables.
  EXPECT_EQ("(([X]+[B])+2)", out_inner_then_x.str());

  std::stringstream out_inner_then_c;
  out_inner_then_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_inner_then_c.str());

  inner_if.end();

  std::stringstream out_inner_end_i;
  out_inner_end_i << i.get_value().get_expr();
  // TODO: Optimize NaryExpr partial expressions.
  EXPECT_EQ("((([I]+1)<7)?([I]+1):([I]+1))", out_inner_end_i.str());

  std::stringstream out_inner_end_j;
  out_inner_end_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[A])", out_inner_end_j.str());

  std::stringstream out_inner_end_k;
  out_inner_end_k << k.get_value().get_expr();
  EXPECT_EQ("((([I]+1)<7)?([K]+3):[K])", out_inner_end_k.str());

  std::stringstream out_inner_end_x;
  out_inner_end_x << x.get_value().get_expr();
  // TODO: Support associative operations over symbolic variables.
  EXPECT_EQ("((([I]+1)<7)?(([X]+[B])+2):([X]+[B]))", out_inner_end_x.str());

  std::stringstream out_inner_end_c;
  out_inner_end_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_inner_end_c.str());

  ok = outer_if.begin_else();
  EXPECT_TRUE(ok);

  i = i + 7;
  j = j + any_int("P");
  x = x + any_int("Q");

  std::stringstream out_outer_else_i;
  out_outer_else_i << i.get_value().get_expr();
  EXPECT_EQ("([I]+7)", out_outer_else_i.str());

  std::stringstream out_outer_else_j;
  out_outer_else_j << j.get_value().get_expr();
  EXPECT_EQ("([J]+[P])", out_outer_else_j.str());

  std::stringstream out_outer_else_k;
  out_outer_else_k << k.get_value().get_expr();
  EXPECT_EQ("[K]", out_outer_else_k.str());

  std::stringstream out_outer_else_x;
  out_outer_else_x << x.get_value().get_expr();
  EXPECT_EQ("([X]+[Q])", out_outer_else_x.str());

  std::stringstream out_outer_else_c;
  out_outer_else_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_outer_else_c.str());

  outer_if.end();

  std::stringstream out_outer_end_i;
  out_outer_end_i << i.get_value().get_expr();
  // TODO: Optimize NaryExpr partial expressions.
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?([I]+1):([I]+1)):([I]+7))", out_outer_end_i.str());

  std::stringstream out_outer_end_j;
  out_outer_end_j << j.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?([J]+[A]):([J]+[P]))", out_outer_end_j.str());

  std::stringstream out_outer_end_k;
  out_outer_end_k << k.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?([K]+3):[K]):[K])", out_outer_end_k.str());

  std::stringstream out_outer_end_x;
  out_outer_end_x << x.get_value().get_expr();
  // TODO: Support associative operations over symbolic variables.
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?(([X]+[B])+2):([X]+[B])):([X]+[Q]))", out_outer_end_x.str());

  std::stringstream out_outer_end_c;
  out_outer_end_c << c.get_value().get_expr();
  EXPECT_EQ("[C]", out_outer_end_c.str());
}
