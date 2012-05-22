#include "loop.h"

std::stack<SharedExpr> Loop::internal_get_stack(const GenericVar* const var_ptr) {
  return begin_expr_map.at(var_ptr);
}

void Loop::internal_stash(const GenericVar* const var_ptr) {
  begin_expr_map[var_ptr].push(var_ptr->get_expr());
}

bool Loop::unwind(const Value<bool>& cond) {
  if(unwind_flag) {
    unwind_flag = unwinding_policy_ptr->unwind(cond);
    if(unwind_flag) {
      internal_unwind(cond);
    } else {
      internal_join();
    }
  }

  return unwind_flag;
}

void Loop::internal_unwind(const Value<bool>& cond) {
  cond_expr_stack.push(cond.get_expr());

  for(GenericVar* var_ptr : var_ptrs) {
    internal_stash(var_ptr);
  }
}

void Loop::internal_join() {
  if(cond_expr_stack.empty()) {
    // loop fell through so nothing needs to be done
    return;
  }

  for(GenericVar* var_ptr : var_ptrs) {
    internal_join(var_ptr);
  }
}

void Loop::internal_join(GenericVar* const var_ptr) {
  // TODO: for clarity we join only one variable at a time. If this turns out to
  // be too expensive, we can optimize by writing a single loop such that no
  // copy of the stack needs to be made.
  std::stack<SharedExpr> cond_expr_stack_copy(cond_expr_stack);
  std::stack<SharedExpr> var_expr_stack = internal_get_stack(var_ptr);

  // Construct If-Then-Else expression in reverse order of loop unwindings.
  // That is, the If-Then-Else expression of the last loop unwinding is
  // constructed first etc. Thus, pop cond_expr_stack_copy to get the inner-most
  // loop condition. Then, use the most recent version of the given symbolic
  // variable before and after the loop unwinding with which the popped
  // condition is associated. The "before" version corresponds to the "else"
  // branch of the new ternary expression whereas the "after" version is
  // assigned to its "then" branch.
  SharedExpr join_expr = var_ptr->get_expr();

  SharedExpr cond_expr;
  SharedExpr before_expr;
  do {

    cond_expr = cond_expr_stack_copy.top();
    before_expr = var_expr_stack.top();

    join_expr = SharedExpr(new TernaryExpr(cond_expr, join_expr, before_expr));

    cond_expr_stack_copy.pop();
    var_expr_stack.pop();

    // stop if either of both stacks is empty
  } while(!(cond_expr_stack_copy.empty() || var_expr_stack.empty()));

  var_ptr->set_expr(join_expr);

  // Once the above optimization is implemented this goes away.
  if(begin_expr_map.empty()) {
    cond_expr_stack = std::stack<SharedExpr>();
  }
}

