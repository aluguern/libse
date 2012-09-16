#include <sstream>
#include "gtest/gtest.h"
#include "expr.h"

using namespace se;

TEST(ExprTest, OperatorsStringConversion) {
  EXPECT_EQ("!", operators[NOT]);
  EXPECT_EQ("+", operators[ADD]);
  EXPECT_EQ("&&", operators[LAND]);
  EXPECT_EQ("||", operators[LOR]);
  EXPECT_EQ("==", operators[EQL]);
  EXPECT_EQ("<", operators[LSS]);
}

TEST(ExprTest, OperatorsOrder) {
  EXPECT_EQ(NOT + 1, ADD);
  EXPECT_EQ(ADD + 1, LAND);
  EXPECT_EQ(LOR + 1, EQL);
  EXPECT_EQ(EQL + 1, LSS);
}

TEST(ExprTest, GetKind) {
  AnyExpr<char> a("A");
  EXPECT_EQ(ANY_EXPR, a.kind());

  ValueExpr<char> b('b');
  EXPECT_EQ(VALUE_EXPR, b.kind());

  CastExpr c(INT, SharedExpr(new AnyExpr<char>("X")));
  EXPECT_EQ(CAST_EXPR, c.kind());

  UnaryExpr d(ADD, SharedExpr(new AnyExpr<char>("X")));
  EXPECT_EQ(UNARY_EXPR, d.kind());

  IfThenElseExpr e(SharedExpr(new AnyExpr<bool>("X")), SharedExpr(new AnyExpr<char>("Y")), SharedExpr(new AnyExpr<char>("Z")));
  EXPECT_EQ(ITE_EXPR, e.kind());

  NaryExpr f(ADD, OperatorInfo<ADD>::attr);
  EXPECT_EQ(NARY_EXPR, f.kind());
}

TEST(ExprTest, GetNameOnAnyExpr) {
  AnyExpr<char> a("That rabbit's dynamite!");
  EXPECT_EQ("That rabbit's dynamite!", a.identifier());
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

  EXPECT_EQ('A', a.value());
  EXPECT_EQ(8, b.value());
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
  NaryExpr a = NaryExpr(ADD, OperatorInfo<ADD>::attr);
  a.append_operand(SharedExpr(new AnyExpr<short>("A")));
  a.append_operand(SharedExpr(new AnyExpr<short>("B")));

  std::stringstream append_out;
  a.write(append_out);

  EXPECT_EQ("([A]+[B])", append_out.str());

  a.prepend_operand(SharedExpr(new AnyExpr<short>("C")));

  std::stringstream prepend_out;
  a.write(prepend_out);

  EXPECT_EQ("([C]+[A]+[B])", prepend_out.str());
}

TEST(ExprTest, NaryExprPrependAndAppend) {
  NaryExpr a = NaryExpr(ADD, OperatorInfo<ADD>::attr);
  a.prepend_operand(SharedExpr(new ValueExpr<int>(3, "C")));
  a.append_operand(SharedExpr(new AnyExpr<short>("A")));
  a.prepend_operand(SharedExpr(new CastExpr(INT, SharedExpr(new ValueExpr<short>(7)))));
  a.append_operand(SharedExpr(new AnyExpr<short>("B")));

  std::stringstream out;
  a.write(out);

  EXPECT_EQ("(((int)(7))+[C:3]+[A]+[B])", out.str());
}

TEST(ExprTest, AddAttr) {
  OperatorAttr expected_attr = LASSOC_ATTR | RASSOC_ATTR | COMM_ATTR | HAS_ID_ELEMENT_ATTR;
  OperatorAttr actual_attr = OperatorInfo<ADD>::attr;
  EXPECT_EQ(expected_attr, actual_attr);
}

TEST(ExprTest, LssAttr) {
  OperatorAttr expected_attr = CLEAR_ATTR;
  OperatorAttr actual_attr = OperatorInfo<LSS>::attr;
  EXPECT_EQ(expected_attr, actual_attr);
}

TEST(ExprTest, GetAttr) {
  NaryExpr a(ADD, OperatorInfo<ADD>::attr);
  EXPECT_TRUE(a.is_commutative());
  EXPECT_TRUE(a.is_associative());

  NaryExpr b(LSS, OperatorInfo<LSS>::attr);
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
  EXPECT_TRUE(get_commutative_attr(OperatorInfo<ADD>::attr));
  EXPECT_TRUE(get_associative_attr(OperatorInfo<ADD>::attr));
  EXPECT_TRUE(get_identity_attr(OperatorInfo<ADD>::attr));

  EXPECT_TRUE(get_commutative_attr(OperatorInfo<LAND>::attr));
  EXPECT_TRUE(get_associative_attr(OperatorInfo<LAND>::attr));
  EXPECT_TRUE(get_identity_attr(OperatorInfo<LAND>::attr));

  EXPECT_TRUE(get_commutative_attr(OperatorInfo<LOR>::attr));
  EXPECT_TRUE(get_associative_attr(OperatorInfo<LOR>::attr));
  EXPECT_TRUE(get_identity_attr(OperatorInfo<LOR>::attr));

  EXPECT_TRUE(get_commutative_attr(OperatorInfo<EQL>::attr));
  EXPECT_TRUE(get_associative_attr(OperatorInfo<EQL>::attr));
  EXPECT_FALSE(get_identity_attr(OperatorInfo<EQL>::attr));

  EXPECT_FALSE(get_commutative_attr(OperatorInfo<LSS>::attr));
  EXPECT_FALSE(get_associative_attr(OperatorInfo<LSS>::attr));
  EXPECT_FALSE(get_identity_attr(OperatorInfo<LSS>::attr));

  EXPECT_TRUE(get_commutative_attr(COMM_ATTR));
  EXPECT_FALSE(get_associative_attr(COMM_ATTR));
  EXPECT_FALSE(get_identity_attr(COMM_ATTR));

  EXPECT_FALSE(get_commutative_attr(LASSOC_ATTR));
  EXPECT_FALSE(get_associative_attr(LASSOC_ATTR));
  EXPECT_FALSE(get_identity_attr(LASSOC_ATTR));

  EXPECT_FALSE(get_commutative_attr(RASSOC_ATTR));
  EXPECT_FALSE(get_associative_attr(RASSOC_ATTR));
  EXPECT_FALSE(get_identity_attr(RASSOC_ATTR));

  EXPECT_FALSE(get_commutative_attr(LASSOC_ATTR | RASSOC_ATTR));
  EXPECT_TRUE(get_associative_attr(LASSOC_ATTR | RASSOC_ATTR));
  EXPECT_FALSE(get_identity_attr(LASSOC_ATTR | RASSOC_ATTR));

  EXPECT_FALSE(get_commutative_attr(CLEAR_ATTR));
  EXPECT_FALSE(get_associative_attr(CLEAR_ATTR));
  EXPECT_FALSE(get_identity_attr(CLEAR_ATTR));
}

TEST(ExprTest, GetAndSetOnUnaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));
  const SharedExpr b = SharedExpr(new AnyExpr<short>("B"));
  UnaryExpr c(NOT, a);
  
  EXPECT_EQ(a, c.operand());
  c.set_operand(b);
  EXPECT_EQ(b, c.operand());
}

TEST(ExprTest, NaryExprAsBinaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));
  const SharedExpr b = SharedExpr(new AnyExpr<short>("B"));
  const SharedExpr c = SharedExpr(new AnyExpr<short>("C"));
  NaryExpr d(ADD, OperatorInfo<ADD>::attr, a, b);
  
  EXPECT_EQ(2, d.operands().size());

  std::stringstream binary_out;
  d.write(binary_out);

  EXPECT_EQ("([A]+[B])", binary_out.str());

  d.append_operand(c);

  EXPECT_EQ(3, d.operands().size());

  std::stringstream nary_out;
  d.write(nary_out);

  EXPECT_EQ("([A]+[B]+[C])", nary_out.str());
}

TEST(ExprTest, NaryExprWriteEmpty) {
  NaryExpr a(ADD, OperatorInfo<ADD>::attr);

  EXPECT_EQ(0, a.operands().size());

  std::stringstream out;
  a.write(out);

  EXPECT_EQ("()", out.str());
}

TEST(ExprTest, NaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));
  const SharedExpr b = SharedExpr(new AnyExpr<short>("B"));
  const SharedExpr c = SharedExpr(new AnyExpr<short>("C"));
  NaryExpr d(ADD, OperatorInfo<ADD>::attr);

  d.append_operand(a);
  d.append_operand(b);
  
  EXPECT_EQ(2, d.operands().size());

  std::stringstream binary_out;
  d.write(binary_out);

  EXPECT_EQ("([A]+[B])", binary_out.str());

  d.append_operand(c);

  EXPECT_EQ(3, d.operands().size());

  std::stringstream nary_out;
  d.write(nary_out);

  EXPECT_EQ("([A]+[B]+[C])", nary_out.str());
}

TEST(ExprTest, GetAndSetOnIfThenElseExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<short>("A"));
  const SharedExpr b = SharedExpr(new AnyExpr<short>("B"));
  const SharedExpr c = SharedExpr(new AnyExpr<short>("C"));
  IfThenElseExpr d(a, b, c);
  
  EXPECT_EQ(a, d.cond_expr());
  EXPECT_EQ(b, d.then_expr());
  EXPECT_EQ(c, d.else_expr());

  d.set_cond_expr(c);
  d.set_then_expr(a);
  d.set_else_expr(b);

  EXPECT_EQ(c, d.cond_expr());
  EXPECT_EQ(a, d.then_expr());
  EXPECT_EQ(b, d.else_expr());
}

TEST(ExprTest, WriteTreeWithoutCast) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(3, "Var_1"));
  const SharedExpr neg = SharedExpr(new UnaryExpr(NOT, a));
  const SharedExpr add = SharedExpr(new NaryExpr(ADD, OperatorInfo<ADD>::attr, a, b));
  const SharedExpr lss = SharedExpr(new NaryExpr(LSS, OperatorInfo<LSS>::attr, add, neg));

  std::stringstream out;
  out << lss;

  EXPECT_EQ("((7+[Var_1:3])<(!7))", out.str());
}

TEST(ExprTest, WriteTreeWithCast) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));
  const SharedExpr cast = SharedExpr(new CastExpr(INT, a));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(3, "Var_1"));
  const SharedExpr neg = SharedExpr(new UnaryExpr(NOT, cast));
  const SharedExpr add = SharedExpr(new NaryExpr(ADD, OperatorInfo<ADD>::attr, a, b));
  const SharedExpr lss = SharedExpr(new NaryExpr(LSS, OperatorInfo<LSS>::attr, add, neg));

  std::stringstream out;
  out << lss;

  EXPECT_EQ("((7+[Var_1:3])<(!((int)(7))))", out.str());
}

TEST(ExprTest, WriteTreeWithTernary) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(3, "Var_1"));
  const SharedExpr c = SharedExpr(new ValueExpr<int>(5, "Var_2"));
  const SharedExpr d = SharedExpr(new NaryExpr(ADD, OperatorInfo<ADD>::attr, b, c));
  const SharedExpr lss = SharedExpr(new NaryExpr(LSS, OperatorInfo<LSS>::attr, a, b));
  const SharedExpr ifThenElse = SharedExpr(new IfThenElseExpr(lss, d, a));

  std::stringstream out;
  out << ifThenElse;

  EXPECT_EQ("((7<[Var_1:3])?([Var_1:3]+[Var_2:5]):7)", out.str());
}

TEST(ExprTest, WriteTreeWithEveryTypeOfExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<int>("A"));
  const SharedExpr b = SharedExpr(new ValueExpr<short>(5));
  const SharedExpr lss = SharedExpr(new NaryExpr(LSS, OperatorInfo<LSS>::attr, a, b));
  const SharedExpr neg = SharedExpr(new UnaryExpr(NOT, lss));
  const SharedExpr c = SharedExpr(new AnyExpr<int>("C"));
  const SharedExpr cast = SharedExpr(new CastExpr(CHAR, c));
  const SharedExpr d = SharedExpr(new AnyExpr<int>("D"));
  const SharedExpr ternary = SharedExpr(new IfThenElseExpr(neg, cast, d));

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

