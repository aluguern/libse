#include "gtest/gtest.h"
#include "interpreter.h"

using namespace sp;

TEST(InterpreterTest, UnsupportedCharVar) {
  SpInterpreter sp_interpreter;

  EXPECT_THROW(sp_interpreter.visit(AnyExpr<char>("A")), InterpreterException);
}

TEST(InterpreterTest, UnsupportedCharValue) {
  SpInterpreter sp_interpreter;

  EXPECT_THROW(sp_interpreter.visit(ValueExpr<char>('A')), InterpreterException);
}

TEST(InterpreterTest, UnsupportedShortIntVar) {
  SpInterpreter sp_interpreter;

  EXPECT_THROW(sp_interpreter.visit(AnyExpr<short int>("A")), InterpreterException);
}

TEST(InterpreterTest, UnsupportedShortIntValue) {
  SpInterpreter sp_interpreter;

  EXPECT_THROW(sp_interpreter.visit(ValueExpr<short int>(5)), InterpreterException);
}

TEST(InterpreterTest, SpInterpreterSatTernaryEquivalence) {
  const SharedExpr a = SharedExpr(new AnyExpr<int>("A"));
  const SharedExpr five = SharedExpr(new ValueExpr<int>(5));
  const SharedExpr lss = SharedExpr(new BinaryExpr(a, five, LSS));
  const SharedExpr neg = SharedExpr(new UnaryExpr(lss, NOT));
  const SharedExpr c = SharedExpr(new AnyExpr<int>("C"));
  const SharedExpr d = SharedExpr(new AnyExpr<int>("D"));
  const SharedExpr ternary = SharedExpr(new TernaryExpr(neg, c, d));

  SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  z3::expr e = sp_interpreter.context.int_const("E");

  solver.add((e == sp_interpreter.walk(ternary)) != 
                ((!(sp_interpreter.walk(a) < 5) && e == sp_interpreter.walk(c)) ||
                    ((sp_interpreter.walk(a) < 5) && e == sp_interpreter.walk(d))));

  // Proves that [e == ((!(a < 5)) ? c : d)] is equivalent to
  // [(!(a < 5) && e == c) || ((a < 5) && e == d)].
  EXPECT_EQ(z3::unsat, solver.check());
}

TEST(InterpreterTest, SpInterpreterUnsatTernaryEquivalence) {
  const SharedExpr a = SharedExpr(new AnyExpr<int>("A"));
  const SharedExpr five = SharedExpr(new ValueExpr<int>(5));
  const SharedExpr lss = SharedExpr(new BinaryExpr(a, five, LSS));
  const SharedExpr neg = SharedExpr(new UnaryExpr(lss, NOT));
  const SharedExpr c = SharedExpr(new AnyExpr<int>("C"));
  const SharedExpr d = SharedExpr(new AnyExpr<int>("D"));
  const SharedExpr ternary = SharedExpr(new TernaryExpr(neg, c, d));

  SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  z3::expr e = sp_interpreter.context.int_const("E");

  solver.add((e == sp_interpreter.walk(ternary)) != 
                ((!(sp_interpreter.walk(a) < 5) && e == sp_interpreter.walk(c)) ||
                    ((sp_interpreter.walk(a) < 5) && e == (sp_interpreter.walk(d) + 1))));

  // Disproves that [e == ((!(a < 5)) ? c : d)] is equivalent to
  // [(!(a < 5) && e == c) || ((a < 5) && e == (d + 1))].
  EXPECT_EQ(z3::sat, solver.check());
}

TEST(InterpreterTest, SpInterpreterWithSatBinaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<int>("A"));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(-2));
  const SharedExpr lss = SharedExpr(new BinaryExpr(a, b, LSS));
  
  SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  solver.add(sp_interpreter.walk(lss));

  solver.check();
  EXPECT_EQ(z3::sat, solver.check());

  std::stringstream out;
  out << solver.get_model();

  EXPECT_EQ("(define-fun A () Int\n  (- 3))", out.str());
}

TEST(InterpreterTest, SpInterpreterWithUnsatBinaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<int>("A"));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(5));
  const SharedExpr lss = SharedExpr(new BinaryExpr(a, b, LSS));
  const SharedExpr neg = SharedExpr(new UnaryExpr(lss, NOT));
  
  SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  solver.add(sp_interpreter.walk(lss));
  solver.add(sp_interpreter.walk(neg));

  EXPECT_EQ(z3::unsat, solver.check());
}

TEST(InterpreterTest, SpInterpreterWithSatTernaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<int>("A"));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(5));
  const SharedExpr lss = SharedExpr(new BinaryExpr(a, b, LSS));
  const SharedExpr neg = SharedExpr(new UnaryExpr(lss, NOT));
  const SharedExpr taut = SharedExpr(new ValueExpr<bool>(true));
  const SharedExpr ternary = SharedExpr(new TernaryExpr(lss, neg, taut));
  
  SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  solver.add(sp_interpreter.walk(ternary));

  EXPECT_EQ(z3::sat, solver.check());
}

TEST(InterpreterTest, SpInterpreterWithUnsatTernaryExpr) {
  const SharedExpr a = SharedExpr(new AnyExpr<int>("A"));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(5));
  const SharedExpr lss = SharedExpr(new BinaryExpr(a, b, LSS));
  const SharedExpr neg = SharedExpr(new UnaryExpr(lss, NOT));
  const SharedExpr falsum = SharedExpr(new ValueExpr<bool>(false));
  const SharedExpr ternary = SharedExpr(new TernaryExpr(lss, neg, falsum));
  
  SpInterpreter sp_interpreter;
  z3::solver solver(sp_interpreter.context);
  solver.add(sp_interpreter.walk(ternary));

  EXPECT_EQ(z3::unsat, solver.check());
}

