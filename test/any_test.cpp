#include "gtest/gtest.h"
#include "overload.h"

TEST(AnyTest, StaticAny) {
  EXPECT_TRUE(any_bool() == any_bool());
  EXPECT_TRUE(any_char() == any_char());
  EXPECT_TRUE(any_int() == any_int());
}

TEST(AnyTest, AnyBool) {
  Bool var = any_bool();
  EXPECT_EQ(any_bool(), var.get_reflect_value());
}

TEST(AnyTest, AnyChar) {
  Char var = any_char();
  EXPECT_EQ(any_char(), var.get_reflect_value());
}

TEST(AnyTest, AnyInt) {
  Int var = any_int();
  EXPECT_EQ(any_int(), var.get_reflect_value());
}

