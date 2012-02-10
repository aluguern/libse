#include <sstream>
#include "gtest/gtest.h"
#include "symtracer.h"

TEST(SymtracerTest, WritePathConstraints) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));
  const SharedExpr b = SharedExpr(new ValueExpr<int>(3, "Var_1"));
  const SharedExpr lss1 = SharedExpr(new BinaryExpr(a, b, LSS));
  const SharedExpr add = SharedExpr(new BinaryExpr(a, b, ADD));
  const SharedExpr lss2 = SharedExpr(new BinaryExpr(add, b, LSS));

  Tracer tracer = Tracer();

  tracer.add_path_constraint(lss1);
  tracer.add_path_constraint(lss2);

  std::stringstream out;
  tracer.write_path_constraints(out);

  EXPECT_EQ("(7<[Var_1:3])\n((7+[Var_1:3])<[Var_1:3])\n", out.str());
}

TEST(SymtracerTest, Reset) {
  const SharedExpr a = SharedExpr(new ValueExpr<short>(7));

  Tracer tracer = Tracer();

  tracer.add_path_constraint(a);

  std::stringstream out;
  tracer.write_path_constraints(out);

  EXPECT_EQ("7\n", out.str());

  tracer.reset();

  std::stringstream new_out;
  tracer.write_path_constraints(new_out);

  EXPECT_EQ("", new_out.str());
}
