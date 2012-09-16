#include <sstream>
#include "gtest/gtest.h"
#include "sequential-se.h"

using namespace se;

TEST(VarTest, BoolTrue) {
  Bool var = true;
  EXPECT_EQ(BOOL, var.type());
  EXPECT_TRUE(var);
}

TEST(VarTest, BoolFalse) {
  Bool var = false;
  EXPECT_EQ(BOOL, var.type());
  EXPECT_FALSE(var);
}

TEST(VarTest, Char) {
  Char var = 3;
  EXPECT_EQ(CHAR, var.type());
  EXPECT_EQ(3, var);
}

TEST(VarTest, Int) {
  Int var = 258;
  EXPECT_EQ(INT, var.type());
  EXPECT_EQ(258, var);
}

TEST(VarTest, ConstBoolTrue) {
  const Bool var = true;
  EXPECT_EQ(BOOL, var.type());
  EXPECT_TRUE(var);
}

TEST(VarTest, ConstBoolFalse) {
  const Bool var = false;
  EXPECT_EQ(BOOL, var.type());
  EXPECT_FALSE(var);
}

TEST(VarTest, ConstChar) {
  const Char var = 3;
  EXPECT_EQ(CHAR, var.type());
  EXPECT_EQ(3, var);
}

TEST(VarTest, ConstInt) {
  const Int var = 258;
  EXPECT_EQ(INT, var.type());
  EXPECT_EQ(258, var);
}

TEST(VarTest, DowncastWithCopyConversionConstructor) {
  Int a = 2;
  Char b = a;

  EXPECT_EQ(2, b);
  EXPECT_EQ(CHAR, b.type());
  EXPECT_TRUE(b.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
}

TEST(VarTest, TransitiveDowncastWithCopyConversionConstructor) {
  Int a = 2;
  Char b = a;
  Int c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(INT, c.type());
  EXPECT_TRUE(c.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());
}

TEST(VarTest, TransitiveDowncastWithCopyConstructor) {
  Int a = 2;
  Char b = a;
  Char c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(CHAR, c.type());
  EXPECT_TRUE(c.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());
}

TEST(VarTest, DowncastWithAssignmentOperator) {
  Int a = 2;
  Char b = 3;
  EXPECT_FALSE(b.is_cast());

  b = a;

  EXPECT_EQ(2, b);
  EXPECT_EQ(CHAR, b.type());
  EXPECT_TRUE(b.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO + 1, b.version());
}

TEST(VarTest, DowncastWithPointerAssignment) {
  Char a = 3;
  EXPECT_FALSE(a.is_cast());

  a = make_value<int>(2);

  EXPECT_EQ(2, a);
  EXPECT_EQ(CHAR, a.type());
  EXPECT_TRUE(a.is_cast());
  EXPECT_EQ(VZERO + 1, a.version());
}

TEST(VarTest, DowncastWithPointerConstructor) {
  Char a = make_value<int>(2);

  EXPECT_EQ(2, a);
  EXPECT_EQ(CHAR, a.type());
  EXPECT_TRUE(a.is_cast());
  EXPECT_EQ(VZERO, a.version());
}

TEST(VarTest, TransitiveDowncastWithAssignmentOperatorAndCopyConversionConstructor) {
  Int a = 2;
  Char b = a;
  Int c = 3;

  c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(INT, c.type());
  EXPECT_TRUE(c.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO + 1, c.version());
}

TEST(VarTest, TransitiveDowncastWithAssignmentOperator) {
  Int a = 2;
  Char b = a;
  Char c = 3;

  c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(CHAR, c.type());
  EXPECT_TRUE(c.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO + 1, c.version());
}

TEST(VarTest, UpcastWithCopyConversionConstructor) {
  Char a = 2;
  Int b = a;

  EXPECT_EQ(2, b);
  EXPECT_EQ(INT, b.type());
  EXPECT_TRUE(b.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
}

TEST(VarTest, TransitiveUpcastWithCopyConversionConstructor) {
  Char a = 2;
  Int b = a;
  Char c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(CHAR, c.type());
  EXPECT_TRUE(c.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());
}

TEST(VarTest, TransitiveUpcastWithCopyConstructor) {
  Char a = 2;
  Int b = a;
  Int c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(INT, c.type());
  EXPECT_TRUE(c.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());
}

TEST(VarTest, UpcastWithAssignmentOperator) {
  Char a = 2;
  Int b = 3;
  EXPECT_FALSE(b.is_cast());

  b = a;

  EXPECT_EQ(2, b);
  EXPECT_EQ(INT, b.type());
  EXPECT_TRUE(b.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO + 1, b.version());
}

TEST(VarTest, UpcastWithPointerAssignment) {
  Int a = 3;
  EXPECT_FALSE(a.is_cast());

  a = make_value<char>(2);

  EXPECT_EQ(2, a);
  EXPECT_EQ(INT, a.type());
  EXPECT_TRUE(a.is_cast());
  EXPECT_EQ(VZERO + 1, a.version());
}

TEST(VarTest, UpcastWithPointerConstructor) {
  Int a = make_value<char>(2);

  EXPECT_EQ(2, a);
  EXPECT_EQ(INT, a.type());
  EXPECT_TRUE(a.is_cast());
  EXPECT_EQ(VZERO, a.version());
}

TEST(VarTest, TransitiveUpcastWithAssignmentOperatorAndCopyConversionConstructor) {
  Char a = 2;
  Int b = a;
  Char c = 3;

  c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(CHAR, c.type());
  EXPECT_TRUE(c.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO + 1, c.version());
}

TEST(VarTest, TransitiveUpcastWithAssignmentOperator) {
  Char a = 2;
  Int b = a;
  Int c = 3;

  c = b;

  EXPECT_EQ(2, c);
  EXPECT_EQ(INT, c.type());
  EXPECT_TRUE(c.is_cast());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO + 1, c.version());
}

TEST(VarTest, NotSymbolicVar) {
  Char var = 3;
  EXPECT_FALSE(var.is_symbolic());
  EXPECT_EQ(VZERO, var.version());
}

TEST(VarTest, SetSymbolic) {
  Char var = 3;

  set_symbolic(var);
  EXPECT_TRUE(var.is_symbolic());
  EXPECT_EQ(VZERO, var.version());
}

TEST(VarTest, SetSymbolicName) {
  Char var = 'A';

  std::string name = "Var_0";
  var.set_symbolic(name);

  EXPECT_TRUE(var.is_symbolic());

  std::stringstream out;
  out << var.value().expr();
  EXPECT_EQ("[Var_0:A]", out.str());

  name.clear();
  std::stringstream after_clear;
  after_clear << var.value().expr();
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

  EXPECT_EQ(INT, a.type());
  EXPECT_EQ(3, a);

  EXPECT_EQ(VZERO, a.version());
}

TEST(VarTest, NotSymbolicAssignmentOperator) {
  Int a = 3;
  Int b = 120;

  b = a;

  EXPECT_EQ(3, a);
  EXPECT_EQ(3, b);
  EXPECT_FALSE(a.is_symbolic());
  EXPECT_FALSE(b.is_symbolic());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO + 1, b.version());
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

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO + 2, b.version());
}

TEST(VarTest, NotSymbolicAssignmentOperatorWithCast) {
  Int a = 3;
  Char b = 120;

  b = a;

  // Typing information is immutable
  EXPECT_EQ(CHAR, b.type());

  EXPECT_EQ(3, a);
  EXPECT_EQ(3, b);
  EXPECT_FALSE(a.is_symbolic());
  EXPECT_FALSE(b.is_symbolic());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO + 1, b.version());
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
  EXPECT_EQ(CHAR, b.type());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO + 2, b.version());
}

TEST(VarTest, NotSymbolicConstructorWithCast) {
  Int a = 3;
  Char b = a + 2;

  EXPECT_EQ(3, a);
  EXPECT_EQ(5, b);
  EXPECT_FALSE(a.is_symbolic());
  EXPECT_FALSE(b.is_symbolic());
  EXPECT_EQ(CHAR, b.type());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
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
  EXPECT_EQ(CHAR, b.type());
  EXPECT_NE(value_ref.expr(), b.value().expr());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
}

TEST(VarTest, NotSymbolicConstructor) {
  Int a = 3;
  Int b = a + 2;

  EXPECT_EQ(3, a);
  EXPECT_EQ(5, b);
  EXPECT_FALSE(a.is_symbolic());
  EXPECT_FALSE(b.is_symbolic());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
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
  // These are not equal due to a partial NaryExpr.
  // EXPECT_EQ(value_ref.expr(), b.value().expr());

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
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

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO + 1, b.version());
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

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO + 2, b.version());
}

TEST(VarTest, SymbolicAssignmentOperatorWithCast) {
  Int a = 2;
  Char b = 120;

  set_symbolic(a);

  EXPECT_FALSE(b.is_symbolic());
  b = a;
  EXPECT_TRUE(b.is_symbolic());

  // Typing information is immutable
  EXPECT_EQ(CHAR, b.type());

  EXPECT_EQ(2, a);
  EXPECT_EQ(2, b);

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO + 1, b.version());
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

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO + 2, b.version());
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

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO + 2, b.version());
}


TEST(VarTest, AddChars) {
  Char a = 2;
  Char b = 3;

  Char c = a + b;
  Char d = b + a;

  EXPECT_EQ(5, c);
  EXPECT_EQ(5, d);

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());
  EXPECT_EQ(VZERO, d.version());
}

TEST(VarTest, AddInts) {
  Int a = 256;
  Int b = 256;
  Int c = a + b;
  Int d = b + a;

  EXPECT_EQ(512, c);
  EXPECT_EQ(512, d);

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());
  EXPECT_EQ(VZERO, d.version());
}

TEST(VarTest, AddCharAndInt) {
  Char a = 3;
  Int b = 5;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, (a + b).type());
  EXPECT_EQ(INT, (b + a).type());

  Int c = a + b;
  Int d = b + a;

  EXPECT_EQ(8, c);
  EXPECT_EQ(8, d);

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());
  EXPECT_EQ(VZERO, d.version());
}

TEST(VarTest, AddCharAndReflectValue) {
  Int a = 2;
  Int b = 3;
  Char c = 4;
  const Value<int>& d = a + b;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, (c + d).type());
  EXPECT_EQ(INT, (d + c).type());

  Int e = c + d;
  Int f = d + c;

  EXPECT_EQ(9, e);
  EXPECT_EQ(9, f);

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());

  EXPECT_EQ(VZERO, e.version());
  EXPECT_EQ(VZERO, f.version());
}

TEST(VarTest, AddCharAndConstant) {
  Char a = 2;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, (a + 3).type());
  EXPECT_EQ(INT, (3 + a).type());

  Int b = a + 3;
  Int c = 3 + a;

  EXPECT_EQ(5, b);
  EXPECT_EQ(5, c);

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());
}

TEST(VarTest, AddReflectValueAndConstant) {
  Char a = 2;
  Int b = 3;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, (a + b + 4).type());
  EXPECT_EQ(INT, (b + a + 4).type());
  EXPECT_EQ(INT, (a + 4 + b).type());
  EXPECT_EQ(INT, (b + 4 + a).type());
  EXPECT_EQ(INT, (4 + a + b).type());
  EXPECT_EQ(INT, (4 + b + a).type());

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

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());
  EXPECT_EQ(VZERO, d.version());
  EXPECT_EQ(VZERO, e.version());
  EXPECT_EQ(VZERO, f.version());
  EXPECT_EQ(VZERO, g.version());
  EXPECT_EQ(VZERO, h.version());
}


TEST(VarTest, AddCharWithConstantAndReflectValue) {
  Char a = 2;
  Char b = 3;
  Char c = 4;
  const Value<char>& d = a + b;

  // C++ guarantees type promotion
  EXPECT_EQ(INT, (c + d + 5).type());
  EXPECT_EQ(INT, (d + c + 5).type());
  EXPECT_EQ(INT, (c + 5 + d).type());
  EXPECT_EQ(INT, (d + 5 + c).type());
  EXPECT_EQ(INT, (5 + c + d).type());
  EXPECT_EQ(INT, (5 + d + c).type());

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

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());

  EXPECT_EQ(VZERO, e.version());
  EXPECT_EQ(VZERO, f.version());
  EXPECT_EQ(VZERO, g.version());
  EXPECT_EQ(VZERO, h.version());
  EXPECT_EQ(VZERO, i.version());
  EXPECT_EQ(VZERO, j.version());
}

TEST(VarTest, LessChars) {
  Char a = 2;
  Char b = 3;

  Bool c = a < b;
  Bool d = b < a;

  EXPECT_TRUE(c);
  EXPECT_FALSE(d);

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());
  EXPECT_EQ(VZERO, c.version());
  EXPECT_EQ(VZERO, d.version());
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

  EXPECT_EQ(VZERO, a.version());
  EXPECT_EQ(VZERO, b.version());

  EXPECT_EQ(VZERO, d.version());
  EXPECT_EQ(VZERO, e.version());
  EXPECT_EQ(VZERO, f.version());
  EXPECT_EQ(VZERO, g.version());
}

TEST(VarTest, GetVersionAfterCopy) {
  Int a = 2;
  a = a + 1;
  EXPECT_EQ(VZERO + 1, a.version());

  Int b = a;
  EXPECT_EQ(VZERO, b.version());
}

TEST(VarTest, GetVersionAfterAssignment) {
  Int a = 2;
  Int b = 1;

  a = a + 1;
  a = a + 1;
  EXPECT_EQ(VZERO + 2, a.version());

  b = a;
  EXPECT_EQ(VZERO + 1, b.version());
}

TEST(VarTest, GetVersionAfterConversion) {
  Char a = 2;
  Int b = 1;

  a = a + 1;
  a = a + 1;
  a = a + 1;
  EXPECT_EQ(VZERO + 3, a.version());

  b = a;
  EXPECT_EQ(VZERO + 1, b.version());
}

TEST(VarTest, GetVersionAfterSetExpr) {
  Int a = 2;
  a.set_expr(SharedExpr());
  EXPECT_EQ(VZERO + 1, a.version());
}

TEST(VarTest, CreateVarName) {
  reset_var_seq();
  EXPECT_EQ("Var_0", create_identifier());
  EXPECT_EQ("Var_1", create_identifier());
  EXPECT_EQ("Var_2", create_identifier());
  EXPECT_EQ("Var_3", create_identifier());

  reset_var_seq();
  EXPECT_EQ("Var_0", create_identifier());
  EXPECT_EQ("Var_1", create_identifier());
  EXPECT_EQ("Var_2", create_identifier());
  EXPECT_EQ("Var_3", create_identifier());
}

TEST(VarTest, UnstashTrueConcreteVar) {
  Int var = 3;
  EXPECT_EQ(VZERO, var.version());
  EXPECT_EQ(3, var);

  var.stash();
  EXPECT_EQ(VZERO, var.version());

  var = 5;
  EXPECT_EQ(VZERO + 1, var.version());
  EXPECT_EQ(5, var);

  var.unstash(true);

  EXPECT_EQ(VZERO + 2, var.version());
  EXPECT_EQ(3, var);
}

TEST(VarTest, UnstashTrueSymbolicVarWithAnyExpr) {
  Int var = any<int>("A");
  EXPECT_EQ(VZERO, var.version());

  std::stringstream out_before;
  out_before << var.value().expr();
  EXPECT_EQ("[A]", out_before.str());

  var.stash();
  EXPECT_EQ(VZERO, var.version());

  var = var + 1;
  EXPECT_EQ(VZERO + 1, var.version());

  std::stringstream out_modified;
  out_modified << var.value().expr();
  EXPECT_EQ("([A]+1)", out_modified.str());

  var.unstash(true);

  EXPECT_EQ(VZERO + 2, var.version());

  std::stringstream out_unstash;
  out_unstash << var.value().expr();
  EXPECT_EQ("[A]", out_unstash.str());
}

TEST(VarTest, UnstashTrueSymbolicVarWithNaryExpr) {
  Int var = any<int>("A");
  var = var + 1;
  EXPECT_EQ(VZERO + 1, var.version());

  std::stringstream out_before;
  out_before << var.value().expr();
  EXPECT_EQ("([A]+1)", out_before.str());

  var.stash();
  EXPECT_EQ(VZERO + 1, var.version());

  var = var + 3;
  EXPECT_EQ(VZERO + 2, var.version());

  std::stringstream out_modified;
  out_modified << var.value().expr();
  EXPECT_EQ("([A]+4)", out_modified.str());

  var.unstash(true);

  EXPECT_EQ(VZERO + 3, var.version());

  std::stringstream out_unstash;
  out_unstash << var.value().expr();
  EXPECT_EQ("([A]+1)", out_unstash.str());
}

TEST(VarTest, UnstashTrueWithoutChanges) {
    Int var = 3;
    EXPECT_EQ(VZERO, var.version());
    EXPECT_EQ(3, var);

    var.stash();
    EXPECT_EQ(VZERO, var.version());

    var.unstash(true);

    EXPECT_EQ(VZERO, var.version());
    EXPECT_EQ(3, var);
}

TEST(VarTest, UnstashFalseConcreteVar) {
  Int var = 3;
  EXPECT_EQ(VZERO, var.version());
  EXPECT_EQ(3, var);

  var.stash();
  EXPECT_EQ(VZERO, var.version());

  var = 5;
  EXPECT_EQ(VZERO + 1, var.version());
  EXPECT_EQ(5, var);

  var.unstash(false);

  // since the flag is false, the version number is not incremented.
  EXPECT_EQ(VZERO + 1, var.version());
  EXPECT_EQ(5, var);
}

TEST(VarTest, UnstashFalseSymbolicVarWithAnyExpr) {
  Int var = any<int>("A");
  EXPECT_EQ(VZERO, var.version());

  std::stringstream out_before;
  out_before << var.value().expr();
  EXPECT_EQ("[A]", out_before.str());

  var.stash();
  EXPECT_EQ(VZERO, var.version());

  var = var + 1;
  EXPECT_EQ(VZERO + 1, var.version());

  std::stringstream out_modified;
  out_modified << var.value().expr();
  EXPECT_EQ("([A]+1)", out_modified.str());

  var.unstash(false);

  // since the flag is false, the version number is not incremented.
  EXPECT_EQ(VZERO + 1, var.version());

  std::stringstream out_unstash;
  out_unstash << var.value().expr();
  EXPECT_EQ("([A]+1)", out_unstash.str());
}

TEST(VarTest, UnstashFalseSymbolicVarWithNaryExpr) {
  Int var = any<int>("A");
  var = var + 1;
  EXPECT_EQ(VZERO + 1, var.version());

  std::stringstream out_before;
  out_before << var.value().expr();
  EXPECT_EQ("([A]+1)", out_before.str());

  var.stash();
  EXPECT_EQ(VZERO + 1, var.version());

  var = var + 3;
  EXPECT_EQ(VZERO + 2, var.version());

  std::stringstream out_modified;
  out_modified << var.value().expr();
  EXPECT_EQ("([A]+4)", out_modified.str());

  var.unstash(false);

  // since the flag is false, the version number is not incremented.
  EXPECT_EQ(VZERO + 2, var.version());

  std::stringstream out_unstash;
  out_unstash << var.value().expr();
  EXPECT_EQ("([A]+4)", out_unstash.str());
}

TEST(VarTest, UnstashFalseWithoutChanges) {
    Int var = 3;
    EXPECT_EQ(VZERO, var.version());
    EXPECT_EQ(3, var);

    var.stash();
    EXPECT_EQ(VZERO, var.version());

    var.unstash(false);

    EXPECT_EQ(VZERO, var.version());
    EXPECT_EQ(3, var);
}