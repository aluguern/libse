#include <z3++.h>

int main(void) {
  z3::context context;

  z3::expr total_order(context.bool_val(true));
  const int N = 700;
  for (int i = 0; i < N; i++) {
    const z3::expr a(context.constant(context.int_symbol(i), context.int_sort()));
    for (int j = i + 1; j < N; j++) {
      const z3::expr b(context.constant(context.int_symbol(j), context.int_sort()));
      total_order = total_order and (a < b or b < a);
    }
  }

  z3::solver solver(context);
  solver.add(total_order);

  const z3::expr x(context.constant(context.int_symbol(2), context.int_sort()));
  const z3::expr y(context.constant(context.int_symbol(5), context.int_sort()));

  solver.add(x < y);

  return z3::unsat == solver.check();
}
