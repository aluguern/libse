#include "gtest/gtest.h"
#include "reflect.h"

TEST(ReflectTest, TypeConversions) {
  const ReflectValue& a = Value<bool>(true);
  const ReflectValue& b = Value<bool>(false);
  const ReflectValue& c = Value<char>('A');
  const ReflectValue& d = Value<int>(42);

  EXPECT_TRUE(static_cast<bool>(a));
  EXPECT_FALSE(static_cast<bool>(b));
  EXPECT_EQ('A', static_cast<char>(c));
  EXPECT_EQ(42, static_cast<int>(d));
}

TEST(ReflectTest, BoolConversion) {
  EXPECT_TRUE(*reflect<bool>(true));
  EXPECT_FALSE(*reflect<bool>(false));
}

class BoolOperator {
private:
  const bool flag;
public:
  BoolOperator(const bool flag) : flag(flag) {}
  operator bool() const { return flag; }
};

TEST(ReflectTest, SmartPointerBoolConversion) {
  EXPECT_FALSE(static_cast<bool>(SmartPointer<BoolOperator>(new BoolOperator(false))));
  EXPECT_TRUE(static_cast<bool>(SmartPointer<BoolOperator>(new BoolOperator(true))));
}

TEST(ReflectTest, Types) {
  const Value<bool> a = Value<bool>(true);
  const Value<char> b = Value<char>(3);
  const Value<int> c = Value<int>(5);

  EXPECT_EQ(BOOL, a.get_type());
  EXPECT_EQ(CHAR, b.get_type());
  EXPECT_EQ(INT, c.get_type());
}

TEST(ReflectTest, Conversion) {
  const ReflectValue* const a = new Value<char>('A');

  char v = *a;
  const char cv = *a;

  EXPECT_EQ('A', v);
  EXPECT_EQ('A', cv);

  delete a;
}

TEST(ReflectTest, PolymorphicConversion) {
  const ReflectValue::Pointer& a = reflect<bool>(true);
  const ReflectValue::Pointer& b = reflect<bool>(false);
  const ReflectValue::Pointer& c = reflect<int>(258);

  bool v = a, w = b;
  int x = c;

  const bool cv = a, cw = b;
  const int cx = c;

  EXPECT_TRUE(v);
  EXPECT_FALSE(w);
  EXPECT_EQ(258, x);

  EXPECT_TRUE(cv);
  EXPECT_FALSE(cw);
  EXPECT_EQ(258, cx);
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

TEST(ReflectTest, ValueCopyConstructorWithExpression) {
  const SharedExpr expr = SharedExpr(new ValueExpr<char>(5));
  const Value<char> a = Value<char>(5, expr);
  const Value<char> b = a;

  EXPECT_EQ(CHAR, b.get_type());
  EXPECT_EQ(5, b.get_value());
  EXPECT_EQ(expr, b.get_expr());
  EXPECT_TRUE(b.is_symbolic());
}

TEST(ReflectTest, ValueCopyConstructorWithNullExpression) {
  const Value<char> a = Value<char>(5, SharedExpr());
  const Value<char> b = a;

  EXPECT_EQ(CHAR, b.get_type());
  EXPECT_EQ(5, b.get_value());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(ReflectTest, ValueAssignmentWithoutExpression) {
  const Value<int> a = Value<int>(5);
  Value<char> b = Value<char>(3);

  b = a;

  // Typing information is immutable
  EXPECT_EQ(CHAR, b.get_type());

  EXPECT_EQ(5, b.get_value());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(ReflectTest, ValueAssignmentWithExpression) {
  const SharedExpr expr = SharedExpr(new ValueExpr<int>(5));
  const Value<int> a = Value<int>(5, expr);
  Value<char> b = Value<char>(3);

  b = a;

  // Typing information is immutable
  EXPECT_EQ(CHAR, b.get_type());

  EXPECT_EQ(5, b.get_value());
  EXPECT_EQ(expr, b.get_expr());
  EXPECT_TRUE(b.is_symbolic());
}

TEST(ReflectTest, ValueAssignmentWithNullExpression) {
  const Value<int> a = Value<int>(5, SharedExpr());
  Value<char> b = Value<char>(3);

  b = a;

  // Typing information is immutable
  EXPECT_EQ(CHAR, b.get_type());

  EXPECT_EQ(5, b.get_value());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(ReflectTest, PolymorphicAssignmentWithoutExpression) {
  const ReflectValue* const a = new Value<int>(5);
  ReflectValue* const b = new Value<char>(3);

  *b = *a;

  // Typing information is immutable
  EXPECT_EQ(CHAR, b->get_type());

  EXPECT_EQ(5, static_cast<Value<char>*>(b)->get_value());
  EXPECT_FALSE(b->is_symbolic());

  delete a;
  delete b;
}

TEST(ReflectTest, PolymorphicAssignmentWithExpression) {
  const SharedExpr expr = SharedExpr(new ValueExpr<int>(5));
  const ReflectValue* const a = new Value<int>(5, expr);
  ReflectValue* const b = new Value<char>(3);

  *b = *a;

  // Typing information is immutable
  EXPECT_EQ(CHAR, b->get_type());

  EXPECT_EQ(5, static_cast<Value<char>*>(b)->get_value());
  EXPECT_EQ(expr, b->get_expr());
  EXPECT_TRUE(b->is_symbolic());

  delete a;
  delete b;
}

TEST(ReflectTest, PolymorphicAssignmentWithNullExpression) {
  const ReflectValue* const a = new Value<int>(5, SharedExpr());
  ReflectValue* const b = new Value<char>(3);

  *b = *a;

  // Typing information is immutable
  EXPECT_EQ(CHAR, b->get_type());

  EXPECT_EQ(5, static_cast<Value<char>*>(b)->get_value());
  EXPECT_FALSE(b->is_symbolic());

  delete a;
  delete b;
}

template<typename T>
const ReflectValue::Pointer reflect_with_expr(const T value) {
  const ReflectValue::Pointer& a = reflect<T>(value);
  a->set_expr(SharedExpr(new ValueExpr<T>(value)));
  return a;
}

TEST(ReflectTest, PolymorphicAddWithoutExpression) {
  const ReflectValue::Pointer& a = reflect<char>(2);
  const ReflectValue::Pointer& b = reflect<int>(5);
  const ReflectValue::Pointer& c = *a + *b;
  const ReflectValue::Pointer& d = *b + *a;

  EXPECT_EQ(INT, c->get_type());
  EXPECT_EQ(INT, d->get_type());

  EXPECT_FALSE(c->is_symbolic());
  EXPECT_FALSE(d->is_symbolic());

  EXPECT_EQ(7, static_cast<Value<int>*>(c.get())->get_value());
  EXPECT_EQ(7, static_cast<Value<int>*>(d.get())->get_value());
}

TEST(ReflectTest, PolymorphicAddWithExpression) {
  const ReflectValue::Pointer& a = reflect_with_expr<int>(2);
  const ReflectValue::Pointer& b = reflect<int>(5);
  const ReflectValue::Pointer& c = *a + *b;
  const ReflectValue::Pointer& d = *b + *a;

  EXPECT_EQ(INT, c->get_type());
  EXPECT_EQ(INT, d->get_type());

  EXPECT_TRUE(c->is_symbolic());
  EXPECT_TRUE(d->is_symbolic());

  EXPECT_EQ(7, static_cast<Value<int>*>(c.get())->get_value());
  EXPECT_EQ(7, static_cast<Value<int>*>(d.get())->get_value());

  std::stringstream out;
  c->get_expr()->write(out);
  EXPECT_EQ("(2+5)", out.str());
}

TEST(ReflectTest, PolymorphicLessThanWithoutExpression) {
  const ReflectValue::Pointer& a = reflect<char>(2);
  const ReflectValue::Pointer& b = reflect<int>(5);
  const ReflectValue::Pointer& c = *a < *b;
  const ReflectValue::Pointer& d = *b < *a;

  EXPECT_EQ(BOOL, c->get_type());
  EXPECT_EQ(BOOL, d->get_type());

  EXPECT_FALSE(c->is_symbolic());
  EXPECT_FALSE(d->is_symbolic());

  EXPECT_TRUE(static_cast<Value<bool>*>(c.get())->get_value());
  EXPECT_FALSE(static_cast<Value<bool>*>(d.get())->get_value());
}

TEST(ReflectTest, PolymorphicLessThanWithExpression) {
  const ReflectValue::Pointer& a = reflect_with_expr<int>(2);
  const ReflectValue::Pointer& b = reflect<int>(5);
  const ReflectValue::Pointer& c = *a < *b;
  const ReflectValue::Pointer& d = *b < *a;

  EXPECT_EQ(BOOL, c->get_type());
  EXPECT_EQ(BOOL, d->get_type());

  EXPECT_TRUE(c->is_symbolic());
  EXPECT_TRUE(d->is_symbolic());

  EXPECT_TRUE(static_cast<Value<bool>*>(c.get())->get_value());
  EXPECT_FALSE(static_cast<Value<bool>*>(d.get())->get_value());

  std::stringstream out;
  c->get_expr()->write(out);
  EXPECT_EQ("(2<5)", out.str());
}

TEST(ReflectTest, SetSymbolic) {
  const ReflectValue::Pointer& a = reflect<int>(5);

  EXPECT_FALSE(a->is_symbolic());
  a->set_symbolic("a");
  EXPECT_TRUE(a->is_symbolic());
}

TEST(ReflectTest, SetSymbolicName) {
  const ReflectValue::Pointer& a = reflect<int>(5);
  std::string name = "Var_0";

  a->set_symbolic(name);

  std::stringstream out;
  a->get_expr()->write(out);
  EXPECT_EQ("[Var_0:5]", out.str());

  name.clear();
  std::stringstream after_clear;
  a->get_expr()->write(after_clear);
  EXPECT_EQ("[Var_0:5]", after_clear.str());
}

TEST(ReflectTest, SetSymbolicTwice) {
  const ReflectValue::Pointer& a = reflect<int>(5);
  a->set_symbolic("a");

  EXPECT_TRUE(a->is_symbolic());
  const SharedExpr& expr = a->get_expr();

  a->set_symbolic("__a");
  EXPECT_TRUE(a->is_symbolic());

  // Thus, symbolic variable name remains the same as well.
  EXPECT_EQ(expr, a->get_expr());
}

TEST(ReflectTest, BitPrecision) {
  // C++ standard does not specify if char is signed or unsigned
  char x = 254;
  int y = 5;
  int z = x + y;

  const ReflectValue::Pointer& a = reflect(x);
  const ReflectValue::Pointer& b = reflect(y);
  const ReflectValue::Pointer& c = *a + *b;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, c->get_type());

  int v = c;
  const int cv = c;

  EXPECT_EQ(z, v);
  EXPECT_EQ(z, cv);
}

TEST(ReflectTest, TypePromotion) {
  // C++ guarantees that a variable of type char is in the range [-128, 127].
  const ReflectValue::Pointer& a = reflect<char>(127);
  const ReflectValue::Pointer& b = reflect<int>(256);
  const ReflectValue::Pointer& c = *a + *b;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, c->get_type());

  int v = c;
  const int cv = c;

  EXPECT_EQ(383, v);
  EXPECT_EQ(383, cv);
}

TEST(ReflectTest, SymbolicBoolConversion) {
  tracer().reset();

  const ReflectValue::Pointer& a = reflect<int>(5);
  a->set_symbolic("A");

  const ReflectValue::Pointer& b = reflect<int>(0);
  b->set_symbolic("B");

  EXPECT_TRUE(*a);
  EXPECT_FALSE(*b);

  std::stringstream out;
  tracer().write_path_constraints(out);

  EXPECT_EQ("[A:5]\n(![B:0])\n", out.str());
}

TEST(ReflectTest, PointerBoolConversion) {
  tracer().reset();

  const ReflectValue::Pointer& a = reflect<int>(5);
  a->set_symbolic("A");

  const ReflectValue::Pointer& b = reflect<int>(0);
  b->set_symbolic("B");

  // C++ forces explicit conversion to bool
  if(a) {};
  if(b) {};

  std::stringstream out;
  tracer().write_path_constraints(out);

  EXPECT_EQ("[A:5]\n(![B:0])\n", out.str());
}

