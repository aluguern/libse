#include "loop.h"

namespace se {

const SharedExpr Loop::NIL_EXPR = SharedExpr(0);

bool Loop::unwind(const Value<bool>& cond) {
  unwind_flag = unwinding_policy_ptr->unwind(cond);
  if(unwind_flag) {
    if(join_expr_map.empty()) {
      const SharedExpr cond_expr = cond.get_expr();
      for(GenericVar* var_ptr : var_ptrs) {
        TernaryExpr* join_expr = new TernaryExpr(cond_expr, NIL_EXPR, var_ptr->get_expr());
        join_expr_map.insert(std::make_pair(var_ptr, join_expr));
      }
    } else {
      internal_unwind(cond);
    }
  } else if(!join_expr_map.empty()) {
      internal_join();
  }

  return unwind_flag;
}

void Loop::internal_unwind(const Value<bool>& cond) {
  for(GenericVar* var_ptr : var_ptrs) {
    internal_current_join(cond, var_ptr);
  }
}

void Loop::internal_current_join(const Value<bool>& cond, GenericVar* var_ptr) {
  TernaryExpr* join_expr = new TernaryExpr(cond.get_expr(), NIL_EXPR, var_ptr->get_expr());
  JoinExprMap::iterator current_join_expr_iter = current_join_expr_map.find(var_ptr);
  if (current_join_expr_iter == current_join_expr_map.end()) {
    current_join_expr_map.insert(std::make_pair(var_ptr, join_expr));

    // link up join_expr_map with current_join_expr_map for the given var_ptr
    JoinExprMap::iterator join_expr_iter = join_expr_map.find(var_ptr);
    to_ternary_expr_ptr(join_expr_iter)->set_then_expr(SharedExpr(join_expr));
  } else {
    to_ternary_expr_ptr(current_join_expr_iter)->set_then_expr(SharedExpr(join_expr));
    current_join_expr_map[var_ptr] = join_expr;
  }
}

void Loop::internal_join() {
  for(GenericVar* var_ptr : var_ptrs) {
    JoinExprMap::iterator join_expr_iter = join_expr_map.find(var_ptr);
    internal_join(var_ptr, to_ternary_expr_ptr(join_expr_iter));
  }
}

void Loop::internal_join(GenericVar* var_ptr, TernaryExpr* join_expr) {
  JoinExprMap::iterator current_join_expr_iter = current_join_expr_map.find(var_ptr);
  if (current_join_expr_iter == current_join_expr_map.end()) {
    join_expr->set_then_expr(var_ptr->get_expr());
  } else {
    TernaryExpr* current_join_expr = to_ternary_expr_ptr(current_join_expr_iter);
    current_join_expr->set_then_expr(var_ptr->get_expr());
  }

  var_ptr->set_expr(SharedExpr(join_expr));
}

inline TernaryExpr* Loop::to_ternary_expr_ptr(const JoinExprMap::iterator& iter) {
  return (*iter).second;
}

}

