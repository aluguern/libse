#include <sstream>
#include "gtest/gtest.h"
#include "sequential-se.h"

using namespace se;

TEST(AnyTest, IsSymbolic) {
  EXPECT_TRUE(any<bool>("A").is_symbolic());
  EXPECT_TRUE(any<char>("A").is_symbolic());
  EXPECT_TRUE(any<int>("A").is_symbolic());
}

TEST(AnyTest, SetSymbolic) {
  const std::string name = "CommonVar";

  Int a = any<int>(name);
  Int b = any<int>(name);

  b = b + 1;

  EXPECT_NE(a, b);
}

TEST(AnyTest, NewObject) {
  const std::string name = "CommonVar";

  Value<bool> a = any<bool>(name);
  Value<bool> b = any<bool>(name);
  EXPECT_NE(a.expr(), b.expr());
  EXPECT_NE(reinterpret_cast<uintptr_t>(&a), reinterpret_cast<uintptr_t>(&b));

  Value<char> c = any<char>(name);
  Value<char> d = any<char>(name);
  EXPECT_NE(c.expr(), d.expr());
  EXPECT_NE(reinterpret_cast<uintptr_t>(&c), reinterpret_cast<uintptr_t>(&d));

  Value<char> e = any<char>(name);
  Value<char> f = any<char>(name);
  EXPECT_NE(e.expr(), f.expr());
  EXPECT_NE(reinterpret_cast<uintptr_t>(&e), reinterpret_cast<uintptr_t>(&f));
}

TEST(AnyTest, AssignAnyBool) {
  Bool var = any<bool>("VAR");
  EXPECT_TRUE(var.is_symbolic());
}

TEST(AnyTest, AssignAnyChar) {
  Char var = any<char>("VAR");
  EXPECT_TRUE(var.is_symbolic());
}

TEST(AnyTest, AssignAnyCharRequiringCast) {
  Int var = any<char>("VAR");
  EXPECT_TRUE(var.is_symbolic());
}

TEST(AnyTest, AnyIntWithCopyConstructor) {
  Value<int> value = any<int>("A");
  EXPECT_TRUE(value.is_symbolic());
}

TEST(AnyTest, AnyIntWithAssignmentOperator) {
  Value<int> a = any<int>("A");
  Value<int> b = any<int>("B");

  EXPECT_NE(a.expr(), b.expr());

  a = b;

  EXPECT_EQ(a.expr(), b.expr());
}

TEST(AnyTest, AnyExpr) {
  Value<bool> a = any<bool>("A");
  Value<char> b = any<char>("B");
  Value<int> c = any<int>("C");

  AnyExpr<bool>* a_ptr = dynamic_cast<AnyExpr<bool>*>(a.expr().get());
  EXPECT_TRUE(a_ptr);

  AnyExpr<char>* b_ptr = dynamic_cast<AnyExpr<char>*>(b.expr().get());
  EXPECT_TRUE(b_ptr);

  AnyExpr<int>* c_ptr = dynamic_cast<AnyExpr<int>*>(c.expr().get());
  EXPECT_TRUE(c_ptr);
}

TEST(AnyTest, OperationOnAnyExpr) {
  Int a = any<int>("A");
  a = a + 2;

  std::stringstream out;
  out << a.value().expr();
  EXPECT_EQ("([A]+2)", out.str());
}

