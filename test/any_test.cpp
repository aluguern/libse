#include "gtest/gtest.h"
#include "overload.h"

TEST(AnyTest, IsSymbolic) {
  EXPECT_TRUE(any_bool("A").is_symbolic());
  EXPECT_TRUE(any_char("A").is_symbolic());
  EXPECT_TRUE(any_int("A").is_symbolic());
}

TEST(AnyTest, SetSymbolic) {
  const std::string name = "CommonVar";

  Int a = any_int(name);
  Int b = any_int(name);

  b = b + 1;

  EXPECT_NE(a, b);
}

TEST(AnyTest, NewObject) {
  const std::string name = "CommonVar";

  Value<bool> a = any_bool(name);
  Value<bool> b = any_bool(name);
  EXPECT_NE(a.get_expr(), b.get_expr());
  EXPECT_NE(reinterpret_cast<uintptr_t>(&a), reinterpret_cast<uintptr_t>(&b));

  Value<char> c = any_char(name);
  Value<char> d = any_char(name);
  EXPECT_NE(c.get_expr(), d.get_expr());
  EXPECT_NE(reinterpret_cast<uintptr_t>(&c), reinterpret_cast<uintptr_t>(&d));

  Value<char> e = any_char(name);
  Value<char> f = any_char(name);
  EXPECT_NE(e.get_expr(), f.get_expr());
  EXPECT_NE(reinterpret_cast<uintptr_t>(&e), reinterpret_cast<uintptr_t>(&f));
}

TEST(AnyTest, AssignAnyBool) {
  Bool var = any_bool("VAR");
  EXPECT_TRUE(var.is_symbolic());
}

TEST(AnyTest, AssignAnyChar) {
  Char var = any_char("VAR");
  EXPECT_TRUE(var.is_symbolic());
}

TEST(AnyTest, AssignAnyCharRequiringCast) {
  Int var = any_char("VAR");
  EXPECT_TRUE(var.is_symbolic());
}

TEST(AnyTest, AnyIntWithCopyConstructor) {
  Value<int> value = any_int("A");
  EXPECT_TRUE(value.is_symbolic());
}

TEST(AnyTest, AnyIntWithAssignmentOperator) {
  Value<int> a = any_int("A");
  Value<int> b = any_int("B");

  EXPECT_NE(a.get_expr(), b.get_expr());

  a = b;

  EXPECT_EQ(a.get_expr(), b.get_expr());
}

