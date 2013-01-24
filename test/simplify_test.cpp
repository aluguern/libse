#include <sstream>
#include "gtest/gtest.h"
#include "sequential-se.h"

// Equality has no unique identity element.
// Therefore, no simplifications are applied.
TEST(SimplifyTest, Equality) {
  se::Int i = se::any<int>("I");

  std::stringstream out;
  out << ((i == 3) == 5).expr();
  EXPECT_EQ("(([I]==3)==5)", out.str());
}

TEST(SimplifyTest, UpdateConcolicVariableWithRHSConstant) {
  se::reset_tracer();

  se::Int var = 3;

  EXPECT_TRUE(var.value().is_concrete());
  EXPECT_FALSE(var.value().has_aggregate());

  se::set_symbolic(var);

  EXPECT_TRUE(var.value().is_concrete());
  EXPECT_FALSE(var.value().has_aggregate());

  var = var + 2;

  EXPECT_TRUE(var.value().is_concrete());
  EXPECT_TRUE(var.value().has_aggregate());
  EXPECT_EQ(5, var.value().data());
  EXPECT_EQ(2, var.value().aggregate());

  std::stringstream out_0;
  out_0 << var.expr();
  EXPECT_EQ("([Var_0:3]+2)", out_0.str());

  var = var + 7;

  EXPECT_TRUE(var.value().is_concrete());
  EXPECT_TRUE(var.value().has_aggregate());
  EXPECT_EQ(12, var.value().data());
  EXPECT_EQ(9, var.value().aggregate());

  std::stringstream out_1;
  out_1 << var.expr();
  EXPECT_EQ("([Var_0:3]+9)", out_1.str());
}

TEST(SimplifyTest, UpdateSymbolicVariableWithRHSConstant) {
  se::Int var = se::any<int>("A");

  EXPECT_FALSE(var.value().is_concrete());
  EXPECT_FALSE(var.value().has_aggregate());

  var = var + 2;

  EXPECT_FALSE(var.value().is_concrete());
  EXPECT_TRUE(var.value().has_aggregate());
  EXPECT_EQ(2, var.value().aggregate());

  std::stringstream out_0;
  out_0 << var.expr();
  EXPECT_EQ("([A]+2)", out_0.str());

  var = var + 7;

  EXPECT_FALSE(var.value().is_concrete());
  EXPECT_TRUE(var.value().has_aggregate());
  EXPECT_EQ(9, var.value().aggregate());

  std::stringstream out_1;
  out_1 << var.expr();
  EXPECT_EQ("([A]+9)", out_1.str());
}

TEST(SimplifyTest, ValueWithRHSConstant) {
  se::reset_tracer();

  se::Int var = se::any<int>("A");

  EXPECT_FALSE(var.value().is_concrete());
  EXPECT_FALSE(var.value().has_aggregate());

  se::Value<int> value = var + 2 + 3;

  EXPECT_TRUE(value.is_concrete());
  EXPECT_TRUE(value.has_aggregate());
  EXPECT_EQ(5, value.aggregate());

  std::stringstream out;
  out << value.expr();
  EXPECT_EQ("([A]+5)", out.str());

  var = value;

  // even though RHS has concrete value it must not propagate
  EXPECT_FALSE(var.value().is_concrete());

  EXPECT_TRUE(var.value().has_aggregate());
  EXPECT_EQ(5, var.value().aggregate());
}

TEST(SimplifyTest, DifferentOperatorsWithRHSConstants) {
  se::Int var = se::any<int>("A");

  std::stringstream out;
  out << ((var + 2 + 7) < 7).expr();

  EXPECT_EQ("(([A]+9)<7)", out.str());
}

