#include <z3++.h>

#include <sstream>
#include <cassert>

int main(void) {
  z3::context context;
  context.set("MBQI_MAX_ITERATIONS", "100");

  z3::solver solver(context);

  const z3::func_decl func_decl(context.function("f",
    context.int_sort(), context.int_sort()));

  const z3::func_decl func_inv_decl(context.function("f-inv",
    context.int_sort(), context.int_sort()));

  const Z3_context ctx = context;
  const Z3_symbol quantifier_symbol = Z3_mk_string_symbol(ctx, "i");
  const Z3_sort quantifier_sort = Z3_mk_int_sort(ctx);
  const z3::expr quantifier_ast(context, Z3_mk_bound(ctx, 0, quantifier_sort)); 
  const z3::expr f_app = func_decl(quantifier_ast);
  const z3::expr finv_app = func_inv_decl(f_app);
  const z3::expr equality_expr = finv_app == quantifier_ast;
  Z3_ast _f_app[1] = { f_app };
  const Z3_pattern pattern = Z3_mk_pattern(ctx, 1, _f_app);
  const Z3_ast injection_ast = Z3_mk_forall(ctx, 
                                 1, /* using default weight */
                                 1, /* number of patterns */
                                 &pattern, /* address of the "array" of patterns */
                                 1, /* number of quantified variables */
                                 &quantifier_sort, 
                                 &quantifier_symbol,
                                 equality_expr);  

  const z3::expr injection_expr(context, injection_ast);
  solver.add(injection_expr);

  std::stringstream out;
  out << injection_expr;
  assert("(forall ((i Int)) (! (= (f-inv (f i)) i) :pattern ((f i))))" == out.str());

  const z3::expr x(context.int_const("x"));
  const z3::expr y(context.int_const("y"));

  solver.add(x == y);
  solver.add(func_decl(x) == func_decl(y));

  const z3::check_result result = solver.check();
  assert(result == z3::unknown);

  return z3::unsat == result;
}
