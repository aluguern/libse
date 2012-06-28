#include <sstream>
#include "gtest/gtest.h"
#include "sequential-se.h"

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

