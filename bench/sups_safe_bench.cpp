#include <z3++.h>
#include <z3.h>

static z3::expr insert(z3::expr set, unsigned element) {
  z3::context& context = set.ctx();
  return z3::store(set, context.int_val(element), context.bool_val(true));
}

int main(void) {
  z3::context context;
  context.set("MACRO_FINDER", "true");

  const z3::sort set_sort(context.array_sort(context.int_sort(), context.bool_sort()));
  const z3::expr empty_set(z3::const_array(context.int_sort(), context.bool_val(false)));

  const z3::expr x(context.constant("x", context.int_sort()));
  const z3::expr y(context.constant("y", context.int_sort()));
  const z3::expr flag(context.constant("flag", context.bool_sort()));
  const z3::expr zero(context.int_val(0));

  const z3::func_decl sup_func_decl(context.function("sup", context.int_sort(), context.int_sort(), context.int_sort()));
  const z3::expr sup_def(z3::forall(x, y, sup_func_decl(x, y) == z3::expr(context, Z3_mk_ite(context, x < y, y, x))));

  const z3::func_decl filter_func_decl(context.function("filter", context.int_sort(), context.bool_sort(), context.int_sort()));
  const z3::expr filter_def(z3::forall(x, flag, filter_func_decl(x, flag) == z3::expr(context, Z3_mk_ite(context, flag, x, zero))));

  const z3::func_decl sups_func_decl(context.function("sups", set_sort, set_sort.array_domain()));
  const z3::expr xs(context.constant("xs", set_sort));

  const int N = 700;
  z3::expr rec_sup_def(zero);
  for (int i = 0; i < N; i++) {
    rec_sup_def = sup_func_decl(rec_sup_def, filter_func_decl(i, z3::select(xs, i)));
  }

  const z3::expr sups_def(z3::forall(xs, sups_func_decl(xs) == rec_sup_def));

  z3::solver solver(context);

  solver.add(sup_def);
  solver.add(filter_def);
  solver.add(sups_def);


  z3::expr set(empty_set);
  for (int k = N - 2; k >= 0; k -= 2) {
    set = insert(set, k);
  }

  solver.add(sups_func_decl(set) == N - 1);

  return z3::sat == solver.check();
}
