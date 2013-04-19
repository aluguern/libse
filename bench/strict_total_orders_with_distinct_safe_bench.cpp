#include <z3++.h>

int main(void) {
  z3::context context;

  const unsigned N = 700;
  Z3_ast x_args[N];

  for (unsigned i = 0; i < N; i++) {
    x_args[i] = context.constant(context.int_symbol(i), context.int_sort());
    Z3_inc_ref(context, x_args[i]);
  }
  const z3::expr x_distinct_expr(context, Z3_mk_distinct(context, N, x_args));

  for (unsigned i = 0; i < N; i++) {
    Z3_dec_ref(context, x_args[i]);
  }

  Z3_ast y_args[N];
  for (unsigned i = 0; i < N; i++) {
    y_args[i] = context.constant(context.int_symbol(N + i), context.int_sort());
    Z3_inc_ref(context, y_args[i]);
  }
  const z3::expr y_distinct_expr(context, Z3_mk_distinct(context, N, y_args));

  for (unsigned i = 0; i < N; i++) {
    Z3_dec_ref(context, y_args[i]);
  }

  z3::solver solver(context);
  solver.add(x_distinct_expr);
  solver.add(y_distinct_expr);

  const z3::expr x(context.constant(context.int_symbol(7), context.int_sort()));
  const z3::expr y(context.constant(context.int_symbol(42), context.int_sort()));

  solver.add(x == y);

  return z3::sat == solver.check();
}
