#include <sstream>
#include "gtest/gtest.h"
#include "sequential-se.h"
#include "interpreter.h"

TEST(MultiPathFunctionalTest, Safe) {
  int MAX = 3;
  se::Int j = se::any_int("K");

  // Unroll loop once to get if statement
  se::Loop if_then_0 = se::Loop(1);
  if_then_0.track(j);
  while(if_then_0.unwind(j < 0)) { j = 0; }

  se::Loop if_then_1 = se::Loop(1);
  if_then_1.track(j);
  while(if_then_1.unwind(j < MAX)) { j = j + MAX; }

  se::SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  solver.add((j < MAX).get_expr()->walk(&sp_interpreter));

  EXPECT_EQ(z3::unsat, solver.check());
}

TEST(MultiPathFunctionalTest, Unsafe) {
  int MAX = 3;
  se::Int j = se::any_int("K");

  // Unroll loop once to get if statement
  se::Loop if_then_1 = se::Loop(1);
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

  se::Loop loop(2);
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

  se::Loop loop(1);
  loop.track(i);
  while(loop.unwind(i == se::Value<bool>(true))) {
    i = i && se::Value<bool>(false);
  }
  
  std::stringstream out;
  out << i.get_value().get_expr();
  EXPECT_EQ("(([I]==1)?([I]&&0):[I])", out.str());
}

