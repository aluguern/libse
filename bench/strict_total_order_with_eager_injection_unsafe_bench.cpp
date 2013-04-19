#include <z3++.h>

int main(void) {
  z3::context context;
  z3::solver solver(context);

  const z3::func_decl func_decl(context.function("f",
    context.int_sort(), context.int_sort()));

  const z3::func_decl func_inv_decl(context.function("f-inv",
    context.int_sort(), context.int_sort()));

  const unsigned N = 700;
  for (unsigned i = 0; i < N; i++) {
    z3::expr i_expr(context.int_val(i));
    solver.add(func_inv_decl(func_decl(i_expr)) == i_expr);
  }

  const z3::expr x(context.int_const("x"));
  const z3::expr y(context.int_const("y"));

  solver.add(x < N && x > 0);
  solver.add(y < N && y > 0);
  solver.add(x == y);
  solver.add(func_decl(x) == func_decl(y));

  return z3::unsat == solver.check();
}
