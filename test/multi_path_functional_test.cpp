#include <sstream>
#include "gtest/gtest.h"
#include "sequential-se.h"
#include "interpreter.h"

TEST(MultiPathFunctionalTest, SafeWithIfThenElse) {
  se::Int j = se::any_int("J");

  se::If branch(j < 0);
  branch.track(j);
  if (branch.begin_then()) { j = 0; }
  if (branch.begin_else()) { j = 1; }
  branch.end();

  se::SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  se::Bool vc = (j < 0) || !(j < 2);
  solver.add(vc.get_expr()->walk(&sp_interpreter));

  EXPECT_EQ(z3::unsat, solver.check());
}

TEST(MultiPathFunctionalTest, UnsafeWithIfThenElse) {
  se::Int j = se::any_int("J");

  se::If branch(j < 0);
  branch.track(j);
  if (branch.begin_then()) { j = 0; }
  if (branch.begin_else()) { j = 1; }
  branch.end();

  se::SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  se::Bool vc = !(j < 1);
  solver.add(vc.get_expr()->walk(&sp_interpreter));

  EXPECT_EQ(z3::sat, solver.check());
}

TEST(MultiPathFunctionalTest, SafeWithIfThen) {
  int MAX = 3;
  se::Int j = se::any_int("K");

  se::If if_0(j < 0);
  if_0.track(j);
  if (if_0.begin_then()) { j = 0; }
  if_0.end();

  se::If if_1(j < MAX);
  if_1.track(j);
  if (if_1.begin_then()) { j = j + MAX; }
  if_1.end();

  se::SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  solver.add((j < MAX).get_expr()->walk(&sp_interpreter));

  EXPECT_EQ(z3::unsat, solver.check());
}

TEST(MultiPathFunctionalTest, UnsafeWithIfThen) {
  int MAX = 3;
  se::Int j = se::any_int("K");

  se::If branch(j < MAX);
  branch.track(j);
  if (branch.begin_then()) { j = j + MAX; }
  branch.end();

  se::SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  solver.add((j < MAX).get_expr()->walk(&sp_interpreter));

  EXPECT_EQ(z3::sat, solver.check());
}

TEST(MultiPathFunctionalTest, SafeWithLoop) {
  int MAX = 3;
  se::Int j = se::any_int("K");

  // Unroll loop once to get if statement
  se::Loop if_then_0(1u);
  if_then_0.track(j);
  while(if_then_0.unwind(j < 0)) { j = 0; }

  se::Loop if_then_1(1u);
  if_then_1.track(j);
  while(if_then_1.unwind(j < MAX)) { j = j + MAX; }

  se::SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  solver.add((j < MAX).get_expr()->walk(&sp_interpreter));

  EXPECT_EQ(z3::unsat, solver.check());
}

TEST(MultiPathFunctionalTest, UnsafeWithLoop) {
  int MAX = 3;
  se::Int j = se::any_int("K");

  // Unroll loop once to get if statement
  se::Loop if_then_1(1u);
  if_then_1.track(j);
  while(if_then_1.unwind(j < MAX)) { j = j + MAX; }

  se::SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  solver.add((j < MAX).get_expr()->walk(&sp_interpreter));

  EXPECT_EQ(z3::sat, solver.check());
}

TEST(MultiPathFunctionalTest, Cast) {
  se::Char i = se::any_char("I");
  se::Int j = se::any_int("J");

  i = j;

  std::stringstream out;
  out << i.get_expr();
  EXPECT_EQ("((char)([J]))", out.str());
}

TEST(MultiPathFunctionalTest, IfThen) {
  int MAX = 3;
  se::Int j = se::any_int("J");

  se::If branch(j < 0);
  branch.track(j);
  if (branch.begin_then()) { j = j + MAX; }
  branch.end();

  std::stringstream out;
  out << j.get_value().get_expr();
  EXPECT_EQ("(([J]<0)?([J]+3):[J])", out.str());
}

TEST(MultiPathFunctionalTest, IfThenElse) {
  int MAX = 3;
  se::Int j = se::any_int("J");

  se::If branch(j < 0);
  branch.track(j);
  if (branch.begin_then()) { j = j + MAX; }
  if (branch.begin_else()) { j = j + se::any_int("A"); }
  branch.end();

  std::stringstream out;
  out << j.get_value().get_expr();
  EXPECT_EQ("(([J]<0)?([J]+3):([J]+[A]))", out.str());
}

TEST(MultiPathFunctionalTest, IfThenElseWithConstantAssignment) {
  se::Int j = se::any_int("J");

  se::If branch(j < 0);
  branch.track(j);
  if (branch.begin_then()) { j = 0; }
  if (branch.begin_else()) { j = 1; }
  branch.end();

  std::stringstream out;
  out << j.get_value().get_expr();
  EXPECT_EQ("(([J]<0)?0:1)", out.str());
}

TEST(MultiPathFunctionalTest, BasicLogicalOperators) {
  se::Bool i = se::any_bool("I");
  se::Bool j = se::any_bool("J");
  se::Bool k = se::any_bool("K");

  i = i && j || k;
  
  std::stringstream out;
  out << i.get_value().get_expr();
  EXPECT_EQ("(([I]&&[J])||[K])", out.str());
}

TEST(MultiPathFunctionalTest, IntUnwind2x) {
  se::Int i = se::any_int("I");

  se::Loop loop(2u);
  loop.track(i);
  while(loop.unwind(i < 5)) {
    i = i + 4;
  }
  
  std::stringstream out;
  out << i.get_value().get_expr();
  EXPECT_EQ("(([I]<5)?((([I]+4)<5)?([I]+8):([I]+4)):[I])", out.str());
}

TEST(MultiPathFunctionalTest, BoolUnwind1x) {
  se::Bool i = se::any_bool("I");

  se::Loop loop(1u);
  loop.track(i);
  while(loop.unwind(i == se::Value<bool>(true))) {
    i = i && se::Value<bool>(false);
  }
  
  std::stringstream out;
  out << i.get_value().get_expr();
  EXPECT_EQ("(([I]==1)?([I]&&0):[I])", out.str());
}

