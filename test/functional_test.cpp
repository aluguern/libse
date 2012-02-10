#include <sstream>
#include "gtest/gtest.h"
#include "var.h"

Int foo(Int other) {
  if(other + 1 < 6) {
    other = other + 42;
  } else {
    other = other + 3;
  }

  return other + 4;
}

TEST(FunctionalTest, IfStatementAndFunctionCall) {
  reset_tracer();

  Int var = 3;
  set_symbolic(var);

  Int i = 0;
  if(i < 9) {
    if(var < 8) {}
    var = var + 2;
    if(var < 7) { var = foo(var); }
  }
  if(var < 5) {}

  std::stringstream out;
  tracer().write_path_constraints(out);

  EXPECT_EQ("([Var_0:3]<8)\n"
            "(([Var_0:3]+2)<7)\n"
            "(!((([Var_0:3]+2)+1)<6))\n"
            "(!(((([Var_0:3]+2)+3)+4)<5))\n", out.str());
}
