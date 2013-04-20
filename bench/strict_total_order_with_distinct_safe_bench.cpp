#include <z3++.h>

int main(void) {
  z3::context context;

  const int N = 700;
  Z3_ast args[N];

  for (unsigned i = 0; i < N; i++) {
    args[i] = context.constant(context.int_symbol(i), context.int_sort());
    Z3_inc_ref(context, args[i]);
  }
  const z3::expr distinct_expr(context, Z3_mk_distinct(context, N, args));

  for (unsigned i = 0; i < N; i++) {
    Z3_dec_ref(context, args[i]);
  }

  z3::solver solver(context);
  solver.add(distinct_expr);

  const z3::expr x(context.constant(context.int_symbol(7), context.int_sort()));
  const z3::expr y(context.constant(context.int_symbol(42), context.int_sort()));

  solver.add(x == y);

  return z3::sat == solver.check();
}
