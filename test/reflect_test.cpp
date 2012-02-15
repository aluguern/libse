#include "gtest/gtest.h"
#include "overload.h"

TEST(ReflectTest, TypeConversions) {
  Value<bool> a = Value<bool>(true);
  Value<bool> b = Value<bool>(false);
  Value<char> c = Value<char>('A');
  Value<int> d = Value<int>(42);

  EXPECT_TRUE(static_cast<bool>(a));
  EXPECT_FALSE(static_cast<bool>(b));
  EXPECT_EQ('A', static_cast<char>(c));
  EXPECT_EQ(42, static_cast<int>(d));

  // test constness conversions
  const Value<bool>& ca = Value<bool>(true);
  const Value<bool>& cb = Value<bool>(false);
  const Value<char>& cc = Value<char>('A');
  const Value<int>& cd = Value<int>(42);

  EXPECT_TRUE(static_cast<bool>(ca));
  EXPECT_FALSE(static_cast<bool>(cb));
  EXPECT_EQ('A', static_cast<char>(cc));
  EXPECT_EQ(42, static_cast<int>(cd));
}

TEST(ReflectTest, BoolConversion) {
  EXPECT_TRUE(reflect<bool>(true));
  EXPECT_FALSE(reflect<bool>(false));
}

class BoolOperator {
private:
  const bool flag;
public:
  BoolOperator(const bool flag) : flag(flag) {}
  operator bool() const { return flag; }
};

TEST(ReflectTest, Types) {
  const Value<bool> a = Value<bool>(true);
  const Value<char> b = Value<char>(3);
  const Value<int> c = Value<int>(5);

  EXPECT_EQ(BOOL, a.get_type());
  EXPECT_EQ(CHAR, b.get_type());
  EXPECT_EQ(INT, c.get_type());
}

TEST(ReflectTest, ValueConstructorWithExpression) {
  const SharedExpr expr = SharedExpr(new ValueExpr<char>(5));
  const Value<char> a = Value<char>(5, expr);

  EXPECT_EQ(CHAR, a.get_type());
  EXPECT_EQ(5, a.get_value());
  EXPECT_TRUE(a.is_symbolic());
}

TEST(ReflectTest, ValueConstructorWithoutExpression) {
  const Value<char> a = Value<char>(5);

  EXPECT_EQ(CHAR, a.get_type());
  EXPECT_EQ(5, a.get_value());
  EXPECT_FALSE(a.is_symbolic());
}

TEST(ReflectTest, ValueConstructorWithNullExpression) {
  const Value<char> a = Value<char>(5, SharedExpr());

  EXPECT_EQ(CHAR, a.get_type());
  EXPECT_EQ(5, a.get_value());
  EXPECT_FALSE(a.is_symbolic());
}

TEST(ReflectTest, ValueCopyConstructorWithoutExpression) {
  const Value<char> a = Value<char>(5);
  const Value<char> b = a;

  EXPECT_EQ(CHAR, b.get_type());
  EXPECT_EQ(5, b.get_value());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(ReflectTest, ValueCopyConstructorWithoutExpressionButCast) {
  const Value<char> a = Value<char>(5);
  const Value<int> b = a;

  EXPECT_EQ(INT, b.get_type());
  EXPECT_EQ(5, b.get_value());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(ReflectTest, ValueCopyConstructorWithExpression) {
  const SharedExpr expr = SharedExpr(new ValueExpr<char>(5));
  const Value<char> a = Value<char>(5, expr);
  const Value<char> b = a;

  EXPECT_EQ(CHAR, b.get_type());
  EXPECT_EQ(5, b.get_value());
  EXPECT_EQ(expr, b.get_expr());
  EXPECT_TRUE(b.is_symbolic());
}

TEST(ReflectTest, ValueCopyConstructorWithExpressionAndCast) {
  const SharedExpr expr = SharedExpr(new ValueExpr<char>(5));
  const Value<char> a = Value<char>(5, expr);
  const Value<int> b = a;

  EXPECT_EQ(INT, b.get_type());
  EXPECT_EQ(5, b.get_value());
  EXPECT_NE(expr, b.get_expr());
  EXPECT_TRUE(b.is_symbolic());
}

TEST(ReflectTest, ValueCopyConstructorWithNullExpression) {
  const Value<char> a = Value<char>(5, SharedExpr());
  const Value<char> b = a;

  EXPECT_EQ(CHAR, b.get_type());
  EXPECT_EQ(5, b.get_value());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(ReflectTest, ValueCopyConstructorWithNullExpressionAndCast) {
  const Value<char> a = Value<char>(5, SharedExpr());
  const Value<int> b = a;

  EXPECT_EQ(INT, b.get_type());
  EXPECT_EQ(5, b.get_value());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(ReflectTest, ValueAssignmentWithoutExpressionButCast) {
  const Value<int> a = Value<int>(5);
  Value<char> b = Value<char>(3);

  b = a;

  // Typing information is immutable
  EXPECT_EQ(CHAR, b.get_type());

  EXPECT_EQ(5, b.get_value());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(ReflectTest, ValueAssignmentWithExpressionAndCast) {
  const SharedExpr expr = SharedExpr(new ValueExpr<int>(5));
  const Value<int> a = Value<int>(5, expr);
  Value<char> b = Value<char>(3);

  b = a;

  // Typing information is immutable
  EXPECT_EQ(CHAR, b.get_type());

  EXPECT_EQ(5, b.get_value());
  EXPECT_NE(expr, b.get_expr());
  EXPECT_TRUE(b.is_symbolic());
}

TEST(ReflectTest, ValueAssignmentWithExpressionAndWithoutCast) {
  const SharedExpr expr = SharedExpr(new ValueExpr<int>(5));
  const Value<int> a = Value<int>(5, expr);
  Value<int> b = Value<int>(3);

  b = a;

  EXPECT_EQ(INT, b.get_type());

  EXPECT_EQ(5, b.get_value());
  EXPECT_EQ(expr, b.get_expr());
  EXPECT_TRUE(b.is_symbolic());
}

TEST(ReflectTest, ValueAssignmentWithNullExpressionAndCast) {
  const Value<int> a = Value<int>(5, SharedExpr());
  Value<char> b = Value<char>(3);

  b = a;

  // Typing information is immutable
  EXPECT_EQ(CHAR, b.get_type());

  EXPECT_EQ(5, b.get_value());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(ReflectTest, ValueAssignmentWithNullExpressionAndWithoutCast) {
  const Value<int> a = Value<int>(5, SharedExpr());
  Value<int> b = Value<int>(3);

  b = a;

  EXPECT_EQ(INT, b.get_type());
  EXPECT_EQ(5, b.get_value());
  EXPECT_FALSE(b.is_symbolic());
}

template<typename T>
const Value<T> reflect_with_expr(const T value) {
  Value<T> a = reflect<T>(value);
  a.set_expr(SharedExpr(new ValueExpr<T>(value)));
  return a;
}

TEST(ReflectTest, AddWithoutExpression) {
  const Value<char>& a = reflect<char>(2);
  const Value<int>& b = reflect<int>(5);
  const ReflectValue& c = a + b;
  const ReflectValue& d = b + a;

  EXPECT_EQ(INT, c.get_type());
  EXPECT_EQ(INT, d.get_type());

  EXPECT_FALSE(c.is_symbolic());
  EXPECT_FALSE(d.is_symbolic());

  EXPECT_EQ(7, (a + b).get_value());
  EXPECT_EQ(7, (b + a).get_value());
}

TEST(ReflectTest, AddWithExpression) {
  const Value<int>& a = reflect_with_expr<int>(2);
  const Value<int>& b = reflect<int>(5);
  const ReflectValue& c = a + b;
  const ReflectValue& d = b + a;

  EXPECT_EQ(INT, c.get_type());
  EXPECT_EQ(INT, d.get_type());

  EXPECT_TRUE(c.is_symbolic());
  EXPECT_TRUE(d.is_symbolic());

  EXPECT_EQ(7, (a + b).get_value());
  EXPECT_EQ(7, (b + a).get_value());

  std::stringstream out;
  c.get_expr()->write(out);
  EXPECT_EQ("(2+5)", out.str());
}

TEST(ReflectTest, LessThanWithoutExpression) {
  const Value<char>& a = reflect<char>(2);
  const Value<int>& b = reflect<int>(5);
  const ReflectValue& c = a < b;
  const ReflectValue& d = b < a;

  EXPECT_EQ(BOOL, c.get_type());
  EXPECT_EQ(BOOL, d.get_type());

  EXPECT_FALSE(c.is_symbolic());
  EXPECT_FALSE(d.is_symbolic());

  EXPECT_TRUE((a < b).get_value());
  EXPECT_FALSE((b < a).get_value());
}

TEST(ReflectTest, LessThanWithExpression) {
  const Value<int>& a = reflect_with_expr<int>(2);
  const Value<int>& b = reflect<int>(5);
  const ReflectValue& c = a < b;
  const ReflectValue& d = b < a;

  EXPECT_EQ(BOOL, c.get_type());
  EXPECT_EQ(BOOL, d.get_type());

  EXPECT_TRUE(c.is_symbolic());
  EXPECT_TRUE(d.is_symbolic());

  EXPECT_TRUE((a < b).get_value());
  EXPECT_FALSE((b < a).get_value());

  std::stringstream out;
  c.get_expr()->write(out);
  EXPECT_EQ("(2<5)", out.str());
}

TEST(ReflectTest, SetSymbolic) {
  Value<int> a = reflect<int>(5);

  EXPECT_FALSE(a.is_symbolic());
  a.set_symbolic("a");
  EXPECT_TRUE(a.is_symbolic());
}

TEST(ReflectTest, SetSymbolicName) {
  Value<int> a = reflect<int>(5);
  std::string name = "Var_0";

  a.set_symbolic(name);

  std::stringstream out;
  a.get_expr()->write(out);
  EXPECT_EQ("[Var_0:5]", out.str());

  name.clear();
  std::stringstream after_clear;
  a.get_expr()->write(after_clear);
  EXPECT_EQ("[Var_0:5]", after_clear.str());
}

TEST(ReflectTest, SetSymbolicTwice) {
  Value<int> a = reflect<int>(5);
  a.set_symbolic("a");

  EXPECT_TRUE(a.is_symbolic());
  const SharedExpr& expr = a.get_expr();

  a.set_symbolic("__a");
  EXPECT_TRUE(a.is_symbolic());

  // Thus, symbolic variable name remains the same as well.
  EXPECT_EQ(expr, a.get_expr());
}

TEST(ReflectTest, NativeBitPrecision) {
  // C++ standard does not specify if char is signed or unsigned
  char x = 254;
  int y = 5;
  int z = x + y;

  const Value<char>& a = reflect(x);
  const Value<int>& b = reflect(y);
  const Value<int>& c = a + b;

  // But C++ guarantees type promotion
  EXPECT_EQ(INT, (a + b).get_type());

  int v = c;
  EXPECT_EQ(z, v);
}

TEST(ReflectTest, TypePromotion) {
  // C++ guarantees that a variable of type char is in the range [-128, 127].
  const Value<char>& a = reflect<char>(127);
  const Value<int>& b = reflect<int>(256);
  const ReflectValue& c = a + b;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, c.get_type());

  int v = a + b;
  int w = b + a;
  EXPECT_EQ(383, v);
  EXPECT_EQ(383, w);
}

TEST(ReflectTest, SymbolicBoolConversion) {
  tracer().reset();

  Value<bool> a = reflect<bool>(true);
  a.set_symbolic("A");

  Value<bool> b = reflect<bool>(false);
  b.set_symbolic("B");

  EXPECT_TRUE(a);
  EXPECT_FALSE(b);

  std::stringstream out;
  tracer().write_path_constraints(out);

  EXPECT_EQ("[A:1]\n(![B:0])\n", out.str());
}

TEST(ReflectTest, SymbolicBoolConversionWithIfStatement) {
  tracer().reset();

  Value<bool> a = reflect<bool>(true);
  a.set_symbolic("A");

  Value<bool> b = reflect<bool>(false);
  b.set_symbolic("B");

  // C++ forces explicit conversion to bool
  if(a) {};
  if(b) {};

  std::stringstream out;
  tracer().write_path_constraints(out);

  EXPECT_EQ("[A:1]\n(![B:0])\n", out.str());
}

