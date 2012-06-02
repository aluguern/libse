#include <sstream>
#include "gtest/gtest.h"
#include "expr.h"

using namespace se;

TEST(ExprTest, GetNameOnAnyExpr) {
  AnyExpr<char> a("That rabbit's dynamite!");
  EXPECT_EQ("That rabbit's dynamite!", a.get_name());
}

TEST(ExprTest, ValueExprWithoutName) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));

  std::stringstream out;
  a->write(out);

  EXPECT_EQ("7", out.str());
}

TEST(ExprTest, GetValueOnValueExpr) {
  ValueExpr<char> a('A');
  ValueExpr<int> b(8);

  EXPECT_EQ('A', a.get_value());
  EXPECT_EQ(8, b.get_value());
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

TEST(ExprTest, GetAndSetOnUnaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));
  const SharedExpr b = SharedExpr(new AnyExpr<short>("B"));
  UnaryExpr c(a, NOT);
  
  EXPECT_EQ(a, c.get_expr());
  c.set_expr(b);
  EXPECT_EQ(b, c.get_expr());
}

TEST(ExprTest, GetAndSetOnBinaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));
  const SharedExpr b = SharedExpr(new AnyExpr<short>("B"));
  BinaryExpr c(a, b, ADD);
  
  EXPECT_EQ(a, c.get_x_expr());
  EXPECT_EQ(b, c.get_y_expr());

  c.set_x_expr(b);
  c.set_y_expr(a);

  EXPECT_EQ(b, c.get_x_expr());
  EXPECT_EQ(a, c.get_y_expr());
}

TEST(ExprTest, GetAndSetOnTernaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));
  const SharedExpr b = SharedExpr(new AnyExpr<short>("B"));
  const SharedExpr c = SharedExpr(new AnyExpr<short>("C"));
  TernaryExpr d(a, b, c);
  
  EXPECT_EQ(a, d.get_cond_expr());
  EXPECT_EQ(b, d.get_then_expr());
  EXPECT_EQ(c, d.get_else_expr());

  d.set_cond_expr(c);
  d.set_then_expr(a);
  d.set_else_expr(b);

  EXPECT_EQ(c, d.get_cond_expr());
  EXPECT_EQ(a, d.get_then_expr());
  EXPECT_EQ(b, d.get_else_expr());
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

TEST(ExprTest, WriteTreeWithEveryTypeOfExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<int>("A"));
  const SharedExpr b = SharedExpr(new ValueExpr<short>(5));
  const SharedExpr lss = SharedExpr(new BinaryExpr(a, b, LSS));
  const SharedExpr neg = SharedExpr(new UnaryExpr(lss, NOT));
  const SharedExpr c = SharedExpr(new AnyExpr<int>("C"));
  const SharedExpr cast = SharedExpr(new CastExpr(c, CHAR));
  const SharedExpr d = SharedExpr(new AnyExpr<int>("D"));
  const SharedExpr ternary = SharedExpr(new TernaryExpr(neg, cast, d));

  std::stringstream out;
  ternary->write(out);

  EXPECT_EQ("((!([A]<5))?((char)([C])):[D])", out.str());
}

// See diagram in include/expr.h
TEST(ExprTest, OperatorEnumLayout) {
  EXPECT_EQ(NOT, UNARY_BEGIN);
  EXPECT_EQ(ADD, UNARY_END);
  EXPECT_EQ(ADD, BINARY_BEGIN);
  EXPECT_EQ(LSS, BINARY_END);
}

// Simple tree postorder traversal to compile-time check Visitor API.
class PostorderVisitor : public Visitor<void> {

public:

  std::stringstream out;

  PostorderVisitor() : out() {}

  void visit(const AnyExpr<bool>& expr) { expr.write(out); }
  void visit(const ValueExpr<bool>& expr) { expr.write(out); }
  void visit(const AnyExpr<char>& expr) { expr.write(out); }
  void visit(const ValueExpr<char>& expr) { expr.write(out); }
  void visit(const AnyExpr<short int>& expr) { expr.write(out); }
  void visit(const ValueExpr<short int>& expr) { expr.write(out); }
  void visit(const AnyExpr<int>& expr) { expr.write(out); }
  void visit(const ValueExpr<int>& expr) { expr.write(out); }

  void visit(const UnaryExpr& expr) {
    expr.get_expr()->walk(this);
    out << operators[expr.get_op()];
  }

  void visit(const BinaryExpr& expr) {
    expr.get_x_expr()->walk(this);
    expr.get_y_expr()->walk(this);
    out << operators[expr.get_op()];
  }

  void visit(const TernaryExpr& expr) {
    expr.get_cond_expr()->walk(this);
    expr.get_then_expr()->walk(this);
    expr.get_else_expr()->walk(this);
  }

  void visit(const CastExpr& expr) {
    expr.get_expr()->walk(this);
    out << types[expr.get_type()];
  }

};

TEST(ExprTest, PostorderVisit) {
  const SharedExpr a = SharedExpr(new AnyExpr<int>("A"));
  const SharedExpr b = SharedExpr(new ValueExpr<short>(5));
  const SharedExpr lss = SharedExpr(new BinaryExpr(a, b, LSS));
  const SharedExpr neg = SharedExpr(new UnaryExpr(lss, NOT));
  const SharedExpr c = SharedExpr(new AnyExpr<int>("C"));
  const SharedExpr cast = SharedExpr(new CastExpr(c, CHAR));
  const SharedExpr d = SharedExpr(new AnyExpr<int>("D"));
  const SharedExpr ternary = SharedExpr(new TernaryExpr(neg, cast, d));

  PostorderVisitor postorder_visitor;
  postorder_visitor.walk(ternary);

  EXPECT_EQ("[A]5<![C]char[D]", postorder_visitor.out.str());
}

