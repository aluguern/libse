#include <sstream>
#include "gtest/gtest.h"
#include "se.h"

se::Int foo(se::Int other) {
  if(other + 1 < 6) {
    other = other + 42;
  } else {
    other = other + 3;
  }

  return other + 4;
}

TEST(FunctionalTest, IfStatementAndFunctionCall) {
  se::reset_tracer();

  se::Int var = 3;
  se::set_symbolic(var);

  se::Int i = 0;
  if(i < 9) {
    if(var < 8) {}
    var = var + 2;
    if(var < 7) { var = foo(var); }
  }
  if(var < 5) {}

  std::stringstream out;
  se::tracer().write_path_constraints(out);

  EXPECT_EQ("([Var_0:3]<8)\n"
            "(([Var_0:3]+2)<7)\n"
            "(!(([Var_0:3]+3)<6))\n"
            "(!(([Var_0:3]+9)<5))\n", out.str());
}
