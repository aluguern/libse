#include <sstream>
#include "gtest/gtest.h"
#include "expr.h"

TEST(ExprTest, ValueExprWithoutName) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));

  std::stringstream out;
  a->write(out);

  EXPECT_EQ("7", out.str());
}

TEST(ExprTest, ValueExprWithName) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7, "A"));

  std::stringstream out;
  a->write(out);

  EXPECT_EQ("[A:7]", out.str());
}

TEST(ExprTest, AnyExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));

  std::stringstream out;
  a->write(out);

  EXPECT_EQ("[A]", out.str());
}

TEST(ExprTest, WriteTreeWithoutCast) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(3, "Var_1"));
  const SharedExpr neg = SharedExpr(new UnaryExpr(a, NOT));
  const SharedExpr add = SharedExpr(new BinaryExpr(a, b, ADD));
  const SharedExpr lss = SharedExpr(new BinaryExpr(add, neg, LSS));

  std::stringstream out;
  lss->write(out);

  EXPECT_EQ("((7+[Var_1:3])<(!7))", out.str());
}

TEST(ExprTest, WriteTreeWithCast) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));
  const SharedExpr cast = SharedExpr(new CastExpr(a, INT));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(3, "Var_1"));
  const SharedExpr neg = SharedExpr(new UnaryExpr(cast, NOT));
  const SharedExpr add = SharedExpr(new BinaryExpr(a, b, ADD));
  const SharedExpr lss = SharedExpr(new BinaryExpr(add, neg, LSS));

  std::stringstream out;
  lss->write(out);

  EXPECT_EQ("((7+[Var_1:3])<(!((int)(7))))", out.str());
}

TEST(ExprTest, WriteTreeWithTernary) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(3, "Var_1"));
  const SharedExpr c = SharedExpr(new ValueExpr<int>(5, "Var_2"));
  const SharedExpr d = SharedExpr(new BinaryExpr(b, c, ADD));
  const SharedExpr lss = SharedExpr(new BinaryExpr(a, b, LSS));
  const SharedExpr ifThenElse = SharedExpr(new TernaryExpr(lss, d, a));

  std::stringstream out;
  ifThenElse->write(out);

  EXPECT_EQ("((7<[Var_1:3])?([Var_1:3]+[Var_2:5]):7)", out.str());
}
