#include "gtest/gtest.h"
#include "overload.h"

TEST(VarTest, BoolTrue) {
  Bool var = true;
  EXPECT_EQ(BOOL, var.get_type());
  EXPECT_TRUE(var);
}

TEST(VarTest, BoolFalse) {
  Bool var = false;
  EXPECT_EQ(BOOL, var.get_type());
  EXPECT_FALSE(var);
}

TEST(VarTest, Char) {
  Char var = 3;
  EXPECT_EQ(CHAR, var.get_type());
  EXPECT_EQ(3, var);
}

TEST(VarTest, Int) {
  Int var = 258;
  EXPECT_EQ(INT, var.get_type());
  EXPECT_EQ(258, var);
}

TEST(VarTest, ConstBoolTrue) {
  const Bool var = true;
  EXPECT_EQ(BOOL, var.get_type());
  EXPECT_TRUE(var);
}

TEST(VarTest, ConstBoolFalse) {
  const Bool var = false;
  EXPECT_EQ(BOOL, var.get_type());
  EXPECT_FALSE(var);
}

TEST(VarTest, ConstChar) {
  const Char var = 3;
  EXPECT_EQ(CHAR, var.get_type());
  EXPECT_EQ(3, var);
}

TEST(VarTest, ConstInt) {
  const Int var = 258;
  EXPECT_EQ(INT, var.get_type());
  EXPECT_EQ(258, var);
}

TEST(VarTest, DowncastWithCopyConversionConstructor) {
  Int a = 2;
  Char b = a;

  EXPECT_EQ(2, b);
  EXPECT_EQ(CHAR, b.get_type());
  EXPECT_TRUE(b.is_cast());
}

TEST(VarTest, TransitiveDowncastWithCopyConversionConstructor) {
  Int a = 2;
  Char b = a;
  Int c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(INT, c.get_type());
  EXPECT_TRUE(c.is_cast());
}

TEST(VarTest, TransitiveDowncastWithCopyConstructor) {
  Int a = 2;
  Char b = a;
  Char c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(CHAR, c.get_type());
  EXPECT_TRUE(c.is_cast());
}

TEST(VarTest, DowncastWithAssignmentOperator) {
  Int a = 2;
  Char b = 3;
  EXPECT_FALSE(b.is_cast());

  b = a;

  EXPECT_EQ(2, b);
  EXPECT_EQ(CHAR, b.get_type());
  EXPECT_TRUE(b.is_cast());
}

TEST(VarTest, DowncastWithPointerAssignment) {
  Char a = 3;
  EXPECT_FALSE(a.is_cast());

  a = reflect<int>(2);

  EXPECT_EQ(2, a);
  EXPECT_EQ(CHAR, a.get_type());
  EXPECT_TRUE(a.is_cast());
}

TEST(VarTest, DowncastWithPointerConstructor) {
  Char a = reflect<int>(2);

  EXPECT_EQ(2, a);
  EXPECT_EQ(CHAR, a.get_type());
  EXPECT_TRUE(a.is_cast());
}

TEST(VarTest, TransitiveDowncastWithAssignmentOperatorAndCopyConversionConstructor) {
  Int a = 2;
  Char b = a;
  Int c = 3;

  c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(INT, c.get_type());
  EXPECT_TRUE(c.is_cast());
}

TEST(VarTest, TransitiveDowncastWithAssignmentOperator) {
  Int a = 2;
  Char b = a;
  Char c = 3;

  c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(CHAR, c.get_type());
  EXPECT_TRUE(c.is_cast());
}

TEST(VarTest, UpcastWithCopyConversionConstructor) {
  Char a = 2;
  Int b = a;

  EXPECT_EQ(2, b);
  EXPECT_EQ(INT, b.get_type());
  EXPECT_TRUE(b.is_cast());
}

TEST(VarTest, TransitiveUpcastWithCopyConversionConstructor) {
  Char a = 2;
  Int b = a;
  Char c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(CHAR, c.get_type());
  EXPECT_TRUE(c.is_cast());
}

TEST(VarTest, TransitiveUpcastWithCopyConstructor) {
  Char a = 2;
  Int b = a;
  Int c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(INT, c.get_type());
  EXPECT_TRUE(c.is_cast());
}

TEST(VarTest, UpcastWithAssignmentOperator) {
  Char a = 2;
  Int b = 3;
  EXPECT_FALSE(b.is_cast());

  b = a;

  EXPECT_EQ(2, b);
  EXPECT_EQ(INT, b.get_type());
  EXPECT_TRUE(b.is_cast());
}

TEST(VarTest, UpcastWithPointerAssignment) {
  Int a = 3;
  EXPECT_FALSE(a.is_cast());

  a = reflect<char>(2);

  EXPECT_EQ(2, a);
  EXPECT_EQ(INT, a.get_type());
  EXPECT_TRUE(a.is_cast());
}

TEST(VarTest, UpcastWithPointerConstructor) {
  Int a = reflect<char>(2);

  EXPECT_EQ(2, a);
  EXPECT_EQ(INT, a.get_type());
  EXPECT_TRUE(a.is_cast());
}

TEST(VarTest, TransitiveUpcastWithAssignmentOperatorAndCopyConversionConstructor) {
  Char a = 2;
  Int b = a;
  Char c = 3;

  c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(CHAR, c.get_type());
  EXPECT_TRUE(c.is_cast());
}

TEST(VarTest, TransitiveUpcastWithAssignmentOperator) {
  Char a = 2;
  Int b = a;
  Int c = 3;

  c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(INT, c.get_type());
  EXPECT_TRUE(c.is_cast());
}

TEST(VarTest, NotSymbolicVar) {
  Char var = 3;
  EXPECT_FALSE(var.is_symbolic());
}

TEST(VarTest, SetSymbolic) {
  Char var = 3;

  set_symbolic(var);
  EXPECT_TRUE(var.is_symbolic());
}

TEST(VarTest, SetSymbolicName) {
  Char var = 'A';

  std::string name = "Var_0";
  var.set_symbolic(name);

  EXPECT_TRUE(var.is_symbolic());

  std::stringstream out;
  var.get_reflect_value().get_expr()->write(out);
  EXPECT_EQ("[Var_0:A]", out.str());

  name.clear();
  std::stringstream after_clear;
  var.get_reflect_value().get_expr()->write(after_clear);
  EXPECT_EQ("[Var_0:A]", after_clear.str());
}

// set_symbolic(T) must compile with built-in types
TEST(VarTest, SetSymbolicBuiltin) {
  bool a = true;
  char b = 2;
  int c = 3;
  // etc.

  set_symbolic(a);
  set_symbolic(b);
  set_symbolic(c);
}

TEST(VarTest, SelfAssignmentOperator) {
  Int a = 3;

  a = a;

  EXPECT_EQ(INT, a.get_type());
  EXPECT_EQ(3, a);
}

TEST(VarTest, NotSymbolicAssignmentOperator) {
  Int a = 3;
  Int b = 120;

  b = a;

  EXPECT_EQ(3, a);
  EXPECT_EQ(3, b);
  EXPECT_FALSE(a.is_symbolic());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(VarTest, NotSymbolicAssignmentOperatorDeep) {
  Int a = 3;
  Int b = 120;

  b = a;
  b = b + 2;

  EXPECT_EQ(3, a);
  EXPECT_EQ(5, b);
  EXPECT_FALSE(a.is_symbolic());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(VarTest, NotSymbolicAssignmentOperatorWithCast) {
  Int a = 3;
  Char b = 120;

  b = a;

  // Typing information is immutable
  EXPECT_EQ(CHAR, b.get_type());

  EXPECT_EQ(3, a);
  EXPECT_EQ(3, b);
  EXPECT_FALSE(a.is_symbolic());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(VarTest, NotSymbolicAssignmentOperatorDeepWithCast) {
  Int a = 3;
  Char b = 120;

  b = a;
  b = b + 2;

  EXPECT_EQ(3, a);
  EXPECT_EQ(5, b);
  EXPECT_FALSE(a.is_symbolic());
  EXPECT_FALSE(b.is_symbolic());
  EXPECT_EQ(CHAR, b.get_type());
}

TEST(VarTest, NotSymbolicConstructorWithCast) {
  Int a = 3;
  Char b = a + 2;

  EXPECT_EQ(3, a);
  EXPECT_EQ(5, b);
  EXPECT_FALSE(a.is_symbolic());
  EXPECT_FALSE(b.is_symbolic());
  EXPECT_EQ(CHAR, b.get_type());
}

TEST(VarTest, SymbolicConstructorWithCast) {
  Int a = 3;
  set_symbolic(a);

  const Value<int>& value_ref = a + 2;
  Char b = value_ref;

  EXPECT_EQ(3, a);
  EXPECT_EQ(5, b);
  EXPECT_TRUE(a.is_symbolic());
  EXPECT_TRUE(b.is_symbolic());
  EXPECT_EQ(CHAR, b.get_type());
  EXPECT_NE(value_ref.get_expr(), b.get_reflect_value().get_expr());
}

TEST(VarTest, NotSymbolicConstructor) {
  Int a = 3;
  Int b = a + 2;

  EXPECT_EQ(3, a);
  EXPECT_EQ(5, b);
  EXPECT_FALSE(a.is_symbolic());
  EXPECT_FALSE(b.is_symbolic());
}

TEST(VarTest, SymbolicConstructor) {
  Int a = 3;
  set_symbolic(a);

  const Value<int>& value_ref = a + 2;
  Int b = value_ref;

  EXPECT_EQ(3, a);
  EXPECT_EQ(5, b);
  EXPECT_TRUE(a.is_symbolic());
  EXPECT_TRUE(b.is_symbolic());
  EXPECT_EQ(value_ref.get_expr(), b.get_reflect_value().get_expr());
}

TEST(VarTest, SymbolicAssignmentOperator) {
  Int a = 2;
  Int b = 120;

  set_symbolic(a);

  EXPECT_FALSE(b.is_symbolic());
  b = a;
  EXPECT_TRUE(b.is_symbolic());

  EXPECT_EQ(2, a);
  EXPECT_EQ(2, b);
}

TEST(VarTest, SymbolicAssignmentOperatorDeep) {
  Int a = 2;
  Int b = 120;

  set_symbolic(a);

  b = a;
  b = b + 3;

  EXPECT_TRUE(b.is_symbolic());
  EXPECT_EQ(2, a);
  EXPECT_EQ(5, b);
}

TEST(VarTest, SymbolicAssignmentOperatorWithCast) {
  Int a = 2;
  Char b = 120;

  set_symbolic(a);

  EXPECT_FALSE(b.is_symbolic());
  b = a;
  EXPECT_TRUE(b.is_symbolic());

  // Typing information is immutable
  EXPECT_EQ(CHAR, b.get_type());

  EXPECT_EQ(2, a);
  EXPECT_EQ(2, b);
}

TEST(VarTest, SymbolicAssignmentOperatorDeepWithCast) {
  Int a = 2;
  Char b = 120;

  set_symbolic(a);

  b = a;
  b = b + 3;

  EXPECT_TRUE(b.is_symbolic());
  EXPECT_EQ(2, a);
  EXPECT_EQ(5, b);
}

TEST(VarTest, NotSymbolicConstructorAndAssignment) {
  Int a = 2;
  Char b = 120;

  set_symbolic(a);

  b = a;
  b = b + 3;

  EXPECT_TRUE(b.is_symbolic());
  EXPECT_EQ(2, a);
  EXPECT_EQ(5, b);
}


TEST(VarTest, AddChars) {
  Char a = 2;
  Char b = 3;

  Char c = a + b;
  Char d = b + a;

  EXPECT_EQ(5, c);
  EXPECT_EQ(5, d);
}

TEST(VarTest, AddInts) {
  Int a = 256;
  Int b = 256;
  Int c = a + b;
  Int d = b + a;

  EXPECT_EQ(512, c);
  EXPECT_EQ(512, d);
}

TEST(VarTest, AddCharAndInt) {
  Char a = 3;
  Int b = 5;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, (a + b).get_type());
  EXPECT_EQ(INT, (b + a).get_type());

  Int c = a + b;
  Int d = b + a;

  EXPECT_EQ(8, c);
  EXPECT_EQ(8, d);
}

TEST(VarTest, AddCharAndReflectValue) {
  Int a = 2;
  Int b = 3;
  Char c = 4;
  const Value<int>& d = a + b;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, (c + d).get_type());
  EXPECT_EQ(INT, (d + c).get_type());

  Int e = c + d;
  Int f = d + c;

  EXPECT_EQ(9, e);
  EXPECT_EQ(9, f);
}

TEST(VarTest, AddCharAndConstant) {
  Char a = 2;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, (a + 3).get_type());
  EXPECT_EQ(INT, (3 + a).get_type());

  Int b = a + 3;
  Int c = 3 + a;

  EXPECT_EQ(5, b);
  EXPECT_EQ(5, c);
}

TEST(VarTest, AddReflectValueAndConstant) {
  Char a = 2;
  Int b = 3;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, (a + b + 4).get_type());
  EXPECT_EQ(INT, (b + a + 4).get_type());
  EXPECT_EQ(INT, (a + 4 + b).get_type());
  EXPECT_EQ(INT, (b + 4 + a).get_type());
  EXPECT_EQ(INT, (4 + a + b).get_type());
  EXPECT_EQ(INT, (4 + b + a).get_type());

  Int c = a + b + 4;
  Int d = b + a + 4;
  Int e = a + 4 + b;
  Int f = b + 4 + a;
  Int g = 4 + a + b;
  Int h = 4 + b + a;

  EXPECT_EQ(9, c);
  EXPECT_EQ(9, d);
  EXPECT_EQ(9, e);
  EXPECT_EQ(9, f);
  EXPECT_EQ(9, g);
  EXPECT_EQ(9, h);
}


TEST(VarTest, AddCharWithConstantAndReflectValue) {
  Char a = 2;
  Char b = 3;
  Char c = 4;
  const Value<char>& d = a + b;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, (c + d + 5).get_type());
  EXPECT_EQ(INT, (d + c + 5).get_type());
  EXPECT_EQ(INT, (c + 5 + d).get_type());
  EXPECT_EQ(INT, (d + 5 + c).get_type());
  EXPECT_EQ(INT, (5 + c + d).get_type());
  EXPECT_EQ(INT, (5 + d + c).get_type());

  Int e = c + d + 5;
  Int f = d + c + 5;
  Int g = c + 5 + d;
  Int h = d + 5 + c;
  Int i = 5 + c + d;
  Int j = 5 + d + c;

  EXPECT_EQ(14, e);
  EXPECT_EQ(14, f);
  EXPECT_EQ(14, g);
  EXPECT_EQ(14, h);
  EXPECT_EQ(14, i);
  EXPECT_EQ(14, j);
}

TEST(VarTest, LessChars) {
  Char a = 2;
  Char b = 3;

  Bool c = a < b;
  Bool d = b < a;

  EXPECT_TRUE(c);
  EXPECT_FALSE(d);
}

TEST(VarTest, LessCharWithConstantAndReflectValue) {
  Int a = 2;
  Int b = 3;
  const Value<int>& c = a + b;

  Bool d = a < 4;
  Bool e = 4 < a;
  Bool f = c < 4;
  Bool g = 4 < c;

  EXPECT_TRUE(d);
  EXPECT_FALSE(e);
  EXPECT_FALSE(f);
  EXPECT_TRUE(g);
}

TEST(VarTest, InitIdWithConcrete) {
  Bool a = true;
  Char b = 'b';
  Int c = 3;

  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&a) == a.get_id());
  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&b) == b.get_id());
  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&c) == c.get_id());
}

TEST(VarTest, InitIdWithAny) {
  Bool a = any_bool("A");
  Char b = any_char("B");
  Int c = any_int("C");

  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&a) == a.get_id());
  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&b) == b.get_id());
  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&c) == c.get_id());
}

TEST(VarTest, InitIdWithVar) {
  Int a = 3;
  Int b = a;

  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&a) == a.get_id());
  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&b) == b.get_id());
}

TEST(VarTest, InitIdWithVarRequiringCast) {
  Bool a = any_bool("A");
  Char b = any_char("B");
  Int c = b;

  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&a) == a.get_id());
  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&b) == b.get_id());
  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&c) == c.get_id());
}

TEST(VarTest, InitIdWithValue) {
  Int a = 3;
  Int b = a + 4;

  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&a) == a.get_id());
  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&b) == b.get_id());
}

TEST(VarTest, InitIdWithValueRequringCast) {
  Int a = reflect<char>('a');

  EXPECT_TRUE(reinterpret_cast<uintptr_t>(&a) == a.get_id());
}

