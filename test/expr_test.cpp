#include <sstream>
#include "gtest/gtest.h"
#include "expr.h"

using namespace se;

TEST(ExprTest, GetKind) {
  AnyExpr<char> a("A");
  EXPECT_EQ(ANY_EXPR, a.get_kind());

  ValueExpr<char> b('b');
  EXPECT_EQ(VALUE_EXPR, b.get_kind());

  CastExpr c(SharedExpr(new AnyExpr<char>("X")), INT);
  EXPECT_EQ(CAST_EXPR, c.get_kind());

  UnaryExpr d(SharedExpr(new AnyExpr<char>("X")), ADD);
  EXPECT_EQ(UNARY_EXPR, d.get_kind());

  TernaryExpr e(SharedExpr(new AnyExpr<bool>("X")), SharedExpr(new AnyExpr<char>("Y")), SharedExpr(new AnyExpr<char>("Z")));
  EXPECT_EQ(TERNARY_EXPR, e.get_kind());

  NaryExpr f(ADD, ReflectOperator<ADD>::attr);
  EXPECT_EQ(NARY_EXPR, f.get_kind());
}

TEST(ExprTest, GetNameOnAnyExpr) {
  AnyExpr<char> a("That rabbit's dynamite!");
  EXPECT_EQ("That rabbit's dynamite!", a.get_name());
}

TEST(ExprTest, ValueExprWithoutName) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));

  std::stringstream out;
  out << a;

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
  out << a;

  EXPECT_EQ("[A:7]", out.str());
}

TEST(ExprTest, AnyExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));

  std::stringstream out;
  out << a;

  EXPECT_EQ("[A]", out.str());
}

TEST(ExprTest, NaryExprAppendAndPrepend) {
  NaryExpr a = NaryExpr(ADD, ReflectOperator<ADD>::attr);
  a.append_expr(SharedExpr(new AnyExpr<short>("A")));
  a.append_expr(SharedExpr(new AnyExpr<short>("B")));

  std::stringstream append_out;
  a.write(append_out);

  EXPECT_EQ("([A]+[B])", append_out.str());

  a.prepend_expr(SharedExpr(new AnyExpr<short>("C")));

  std::stringstream prepend_out;
  a.write(prepend_out);

  EXPECT_EQ("([C]+[A]+[B])", prepend_out.str());
}

TEST(ExprTest, NaryExprPrependAndAppend) {
  NaryExpr a = NaryExpr(ADD, ReflectOperator<ADD>::attr);
  a.prepend_expr(SharedExpr(new ValueExpr<int>(3, "C")));
  a.append_expr(SharedExpr(new AnyExpr<short>("A")));
  a.prepend_expr(SharedExpr(new CastExpr(SharedExpr(new ValueExpr<short>(7)), INT)));
  a.append_expr(SharedExpr(new AnyExpr<short>("B")));

  std::stringstream out;
  a.write(out);

  EXPECT_EQ("(((int)(7))+[C:3]+[A]+[B])", out.str());
}

TEST(ExprTest, AddAttr) {
  OperatorAttr expected_attr = LASSOC_ATTR | RASSOC_ATTR | COMM_ATTR;
  OperatorAttr actual_attr = ReflectOperator<ADD>::attr;
  EXPECT_EQ(expected_attr, actual_attr);
}

TEST(ExprTest, LssAttr) {
  OperatorAttr expected_attr = CLEAR_ATTR;
  OperatorAttr actual_attr = ReflectOperator<LSS>::attr;
  EXPECT_EQ(expected_attr, actual_attr);
}

TEST(ExprTest, GetAttr) {
  NaryExpr a(ADD, ReflectOperator<ADD>::attr);
  EXPECT_TRUE(a.is_commutative());
  EXPECT_TRUE(a.is_associative());

  NaryExpr b(LSS, ReflectOperator<LSS>::attr);
  EXPECT_FALSE(b.is_commutative());
  EXPECT_FALSE(b.is_associative());

  NaryExpr c(ADD, COMM_ATTR);
  EXPECT_TRUE(c.is_commutative());
  EXPECT_FALSE(c.is_associative());

  NaryExpr d(ADD, LASSOC_ATTR);
  EXPECT_FALSE(d.is_commutative());
  EXPECT_FALSE(d.is_associative());

  NaryExpr e(ADD, RASSOC_ATTR);
  EXPECT_FALSE(e.is_commutative());
  EXPECT_FALSE(e.is_associative());

  NaryExpr f(ADD, LASSOC_ATTR | RASSOC_ATTR);
  EXPECT_FALSE(f.is_commutative());
  EXPECT_TRUE(f.is_associative());

  NaryExpr g(LSS, CLEAR_ATTR);
  EXPECT_FALSE(g.is_commutative());
  EXPECT_FALSE(g.is_associative());
}

TEST(ExprTest, AttrFunctions) {
  EXPECT_TRUE(get_commutative_attr(ReflectOperator<ADD>::attr));
  EXPECT_TRUE(get_associative_attr(ReflectOperator<ADD>::attr));

  EXPECT_FALSE(get_commutative_attr(ReflectOperator<LSS>::attr));
  EXPECT_FALSE(get_associative_attr(ReflectOperator<LSS>::attr));

  EXPECT_TRUE(get_commutative_attr(COMM_ATTR));
  EXPECT_FALSE(get_associative_attr(COMM_ATTR));

  EXPECT_FALSE(get_commutative_attr(LASSOC_ATTR));
  EXPECT_FALSE(get_associative_attr(LASSOC_ATTR));

  EXPECT_FALSE(get_commutative_attr(RASSOC_ATTR));
  EXPECT_FALSE(get_associative_attr(RASSOC_ATTR));

  EXPECT_FALSE(get_commutative_attr(LASSOC_ATTR | RASSOC_ATTR));
  EXPECT_TRUE(get_associative_attr(LASSOC_ATTR | RASSOC_ATTR));

  EXPECT_FALSE(get_commutative_attr(CLEAR_ATTR));
  EXPECT_FALSE(get_associative_attr(CLEAR_ATTR));
}

TEST(ExprTest, GetAndSetOnUnaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));
  const SharedExpr b = SharedExpr(new AnyExpr<short>("B"));
  UnaryExpr c(a, NOT);
  
  EXPECT_EQ(a, c.get_expr());
  c.set_expr(b);
  EXPECT_EQ(b, c.get_expr());
}

TEST(ExprTest, NaryExprAsBinaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));
  const SharedExpr b = SharedExpr(new AnyExpr<short>("B"));
  const SharedExpr c = SharedExpr(new AnyExpr<short>("C"));
  NaryExpr d(ADD, ReflectOperator<ADD>::attr, a, b);
  
  EXPECT_EQ(2, d.get_exprs().size());

  std::stringstream binary_out;
  d.write(binary_out);

  EXPECT_EQ("([A]+[B])", binary_out.str());

  d.append_expr(c);

  EXPECT_EQ(3, d.get_exprs().size());

  std::stringstream nary_out;
  d.write(nary_out);

  EXPECT_EQ("([A]+[B]+[C])", nary_out.str());
}

TEST(ExprTest, NaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));
  const SharedExpr b = SharedExpr(new AnyExpr<short>("B"));
  const SharedExpr c = SharedExpr(new AnyExpr<short>("C"));
  NaryExpr d(ADD, ReflectOperator<ADD>::attr);

  d.append_expr(a);
  d.append_expr(b);
  
  EXPECT_EQ(2, d.get_exprs().size());

  std::stringstream binary_out;
  d.write(binary_out);

  EXPECT_EQ("([A]+[B])", binary_out.str());

  d.append_expr(c);

  EXPECT_EQ(3, d.get_exprs().size());

  std::stringstream nary_out;
  d.write(nary_out);

  EXPECT_EQ("([A]+[B]+[C])", nary_out.str());
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
  const SharedExpr add = SharedExpr(new NaryExpr(ADD, ReflectOperator<ADD>::attr, a, b));
  const SharedExpr lss = SharedExpr(new NaryExpr(LSS, ReflectOperator<LSS>::attr, add, neg));

  std::stringstream out;
  out << lss;

  EXPECT_EQ("((7+[Var_1:3])<(!7))", out.str());
}

TEST(ExprTest, WriteTreeWithCast) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));
  const SharedExpr cast = SharedExpr(new CastExpr(a, INT));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(3, "Var_1"));
  const SharedExpr neg = SharedExpr(new UnaryExpr(cast, NOT));
  const SharedExpr add = SharedExpr(new NaryExpr(ADD, ReflectOperator<ADD>::attr, a, b));
  const SharedExpr lss = SharedExpr(new NaryExpr(LSS, ReflectOperator<LSS>::attr, add, neg));

  std::stringstream out;
  out << lss;

  EXPECT_EQ("((7+[Var_1:3])<(!((int)(7))))", out.str());
}

TEST(ExprTest, WriteTreeWithTernary) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(3, "Var_1"));
  const SharedExpr c = SharedExpr(new ValueExpr<int>(5, "Var_2"));
  const SharedExpr d = SharedExpr(new NaryExpr(ADD, ReflectOperator<ADD>::attr, b, c));
  const SharedExpr lss = SharedExpr(new NaryExpr(LSS, ReflectOperator<LSS>::attr, a, b));
  const SharedExpr ifThenElse = SharedExpr(new TernaryExpr(lss, d, a));

  std::stringstream out;
  out << ifThenElse;

  EXPECT_EQ("((7<[Var_1:3])?([Var_1:3]+[Var_2:5]):7)", out.str());
}

TEST(ExprTest, WriteTreeWithEveryTypeOfExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<int>("A"));
  const SharedExpr b = SharedExpr(new ValueExpr<short>(5));
  const SharedExpr lss = SharedExpr(new NaryExpr(LSS, ReflectOperator<LSS>::attr, a, b));
  const SharedExpr neg = SharedExpr(new UnaryExpr(lss, NOT));
  const SharedExpr c = SharedExpr(new AnyExpr<int>("C"));
  const SharedExpr cast = SharedExpr(new CastExpr(c, CHAR));
  const SharedExpr d = SharedExpr(new AnyExpr<int>("D"));
  const SharedExpr ternary = SharedExpr(new TernaryExpr(neg, cast, d));

  std::stringstream out;
  out << ternary;

  EXPECT_EQ("((!([A]<5))?((char)([C])):[D])", out.str());
}

// See diagram in include/expr.h
TEST(ExprTest, OperatorEnumLayout) {
  EXPECT_EQ(NOT, UNARY_BEGIN);
  EXPECT_EQ(ADD, UNARY_END);
  EXPECT_EQ(ADD, NARY_BEGIN);
  EXPECT_EQ(LSS, NARY_END);
}

