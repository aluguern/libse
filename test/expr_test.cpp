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

  ArrayExpr g(CHAR, 5, "F");
  EXPECT_EQ(ARRAY_EXPR, g.kind());

  const SharedExpr index_expr(new ValueExpr<size_t>(2));
  SelectExpr h(SharedExpr(new ArrayExpr(CHAR, 5, "F")), index_expr);
  EXPECT_EQ(SELECT_EXPR, h.kind());

  StoreExpr i(SharedExpr(new ArrayExpr(CHAR, 5, "F")), index_expr,
    SharedExpr(new AnyExpr<char>("X")));
  EXPECT_EQ(STORE_EXPR, i.kind());
}

TEST(ExprTest, GetNameOnAnyExpr) {
  AnyExpr<char> a("That rabbit's dynamite!");
  EXPECT_EQ("That rabbit's dynamite!", a.identifier());
}

TEST(ExprTest, GetNameOnArrayExpr) {
  ArrayExpr a(INT, 7, "That rabbit's dynamite!");
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

TEST(ExprTest, ArrayExpr) {
  const SharedExpr array_expr(new ArrayExpr(CHAR, 5, "F"));

  std::stringstream out;
  out << array_expr;

  EXPECT_EQ("[F]", out.str());
}

TEST(ExprTest, SelectExpr) {
  const SharedExpr array_expr(new ArrayExpr(CHAR, 5, "F"));
  const SharedExpr index_expr(new ValueExpr<size_t>(2));
  const SharedExpr select_expr(new SelectExpr(array_expr, index_expr));

  std::stringstream out;
  out << select_expr;

  EXPECT_EQ("Select([F], 2)", out.str());
}

TEST(ExprTest, StoreExpr) {
  const SharedExpr array_expr(new ArrayExpr(CHAR, 5, "F"));
  const SharedExpr elem_expr(new AnyExpr<char>("X"));
  const SharedExpr index_expr(new ValueExpr<size_t>(2));
  const SharedExpr store_expr(new StoreExpr(array_expr, index_expr, elem_expr));

  std::stringstream out;
  out << store_expr;

  EXPECT_EQ("Store([F], 2, [X])", out.str());
}

TEST(ExprTest, GetOnArrayExpr) {
  const ArrayExpr array_expr(CHAR, 5, "F");
  
  EXPECT_EQ(CHAR, array_expr.range_type());
  EXPECT_EQ(5, array_expr.size());
  EXPECT_EQ("F", array_expr.identifier());
}

TEST(ExprTest, GetOnSelectExpr) {
  const SharedExpr array_expr(new ArrayExpr(CHAR, 5, "F"));
  const SharedExpr index_expr(new ValueExpr<size_t>(2));
  const SelectExpr select_expr(array_expr, index_expr);
  
  EXPECT_EQ(array_expr, select_expr.array_expr());
  EXPECT_EQ(index_expr, select_expr.index_expr());
}

TEST(ExprTest, GetOnStoreExpr) {
  const SharedExpr array_expr(new ArrayExpr(CHAR, 5, "F"));
  const SharedExpr index_expr(new ValueExpr<size_t>(2));
  const SharedExpr elem_expr(new AnyExpr<char>("X"));
  const StoreExpr store_expr(array_expr, index_expr, elem_expr);
  
  EXPECT_EQ(array_expr, store_expr.array_expr());
  EXPECT_EQ(index_expr, store_expr.index_expr());
  EXPECT_EQ(elem_expr, store_expr.elem_expr());
}
