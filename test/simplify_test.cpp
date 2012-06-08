#include <sstream>
#include "gtest/gtest.h"
#include "se.h"

TEST(SimplifyTest, UpdateConcolicVariableWithRHSConstant) {
  se::reset_tracer();

  se::Int var = 3;

  EXPECT_TRUE(var.get_value().has_conv_support());
  EXPECT_FALSE(var.get_value().has_aux_value());

  se::set_symbolic(var);

  EXPECT_TRUE(var.get_value().has_conv_support());
  EXPECT_FALSE(var.get_value().has_aux_value());

  var = var + 2;

  EXPECT_TRUE(var.get_value().has_conv_support());
  EXPECT_TRUE(var.get_value().has_aux_value());
  EXPECT_EQ(5, var.get_value().get_value());
  EXPECT_EQ(2, var.get_value().get_aux_value());

  std::stringstream out_0;
  var.get_expr()->write(out_0);
  EXPECT_EQ("([Var_0:3]+2)", out_0.str());

  var = var + 7;

  EXPECT_TRUE(var.get_value().has_conv_support());
  EXPECT_TRUE(var.get_value().has_aux_value());
  EXPECT_EQ(12, var.get_value().get_value());
  EXPECT_EQ(9, var.get_value().get_aux_value());

  std::stringstream out_1;
  var.get_expr()->write(out_1);
  EXPECT_EQ("([Var_0:3]+9)", out_1.str());
}

TEST(SimplifyTest, UpdateSymbolicVariableWithRHSConstant) {
  se::Int var = se::any_int("A");

  EXPECT_FALSE(var.get_value().has_conv_support());
  EXPECT_FALSE(var.get_value().has_aux_value());

  var = var + 2;

  EXPECT_FALSE(var.get_value().has_conv_support());
  EXPECT_TRUE(var.get_value().has_aux_value());
  EXPECT_EQ(2, var.get_value().get_aux_value());

  std::stringstream out_0;
  var.get_expr()->write(out_0);
  EXPECT_EQ("([A]+2)", out_0.str());

  var = var + 7;

  EXPECT_FALSE(var.get_value().has_conv_support());
  EXPECT_TRUE(var.get_value().has_aux_value());
  EXPECT_EQ(9, var.get_value().get_aux_value());

  std::stringstream out_1;
  var.get_expr()->write(out_1);
  EXPECT_EQ("([A]+9)", out_1.str());
}

TEST(SimplifyTest, ValueWithRHSConstant) {
  se::reset_tracer();

  se::Int var = se::any_int("A");

  EXPECT_FALSE(var.get_value().has_conv_support());
  EXPECT_FALSE(var.get_value().has_aux_value());

  se::Value<int> value = var + 2 + 3;

  EXPECT_TRUE(value.has_conv_support());
  EXPECT_TRUE(value.has_aux_value());
  EXPECT_EQ(5, value.get_aux_value());

  std::stringstream out;
  value.get_expr()->write(out);
  EXPECT_EQ("([A]+5)", out.str());

  var = value;

  // even though RHS has conv support it must not propagate
  EXPECT_FALSE(var.get_value().has_conv_support());

  EXPECT_TRUE(var.get_value().has_aux_value());
  EXPECT_EQ(5, var.get_value().get_aux_value());
}

TEST(SimplifyTest, DifferentOperatorsWithRHSConstants) {
  se::Int var = se::any_int("A");

  std::stringstream out;
  ((var + 2 + 7) < 7).get_expr()->write(out);

  EXPECT_EQ("(([A]+9)<7)", out.str());
}

