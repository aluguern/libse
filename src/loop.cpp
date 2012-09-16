// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "loop.h"

namespace se {

const SharedExpr Loop::NIL_EXPR = SharedExpr(0);

bool Loop::unwind(const Value<bool>& cond) { 
  if(cond.is_symbolic()) {
    unwind_flag = unwinding_policy_ptr->unwind(cond);
    if(unwind_flag) {
      if(join_expr_map.empty()) {
        internal_init(cond);
      } else {
        internal_unwind(cond);
      }
    } else if(!join_expr_map.empty()) {
      internal_join();
    }
    return unwind_flag;
  } else {
    return cond.value();
  }
}

void Loop::internal_init(const Value<bool>& cond) {
  const SharedExpr cond_expr = cond.expr();
  for(AbstractVar* var_ptr : var_ptrs) {
    IfThenElseExpr* join_expr = new IfThenElseExpr(cond_expr, NIL_EXPR, var_ptr->expr());
    join_expr_map.insert(std::make_pair(var_ptr, join_expr));
  } 
}

void Loop::internal_unwind(const Value<bool>& cond) {
  for(AbstractVar* var_ptr : var_ptrs) {
    internal_current_join(cond, var_ptr);
  }
}

void Loop::internal_current_join(const Value<bool>& cond, AbstractVar* var_ptr) {
  IfThenElseExpr* join_expr = new IfThenElseExpr(cond.expr(), NIL_EXPR, var_ptr->expr());
  JoinExprMap::iterator current_join_expr_iter = current_join_expr_map.find(var_ptr);
  if (current_join_expr_iter == current_join_expr_map.end()) {
    current_join_expr_map.insert(std::make_pair(var_ptr, join_expr));

    // link up join_expr_map with current_join_expr_map for the given var_ptr
    JoinExprMap::iterator join_expr_iter = join_expr_map.find(var_ptr);
    to_ite_expr_ptr(join_expr_iter)->set_then_expr(SharedExpr(join_expr));
  } else {
    to_ite_expr_ptr(current_join_expr_iter)->set_then_expr(SharedExpr(join_expr));
    current_join_expr_map[var_ptr] = join_expr;
  }
}

void Loop::internal_join() {
  for(AbstractVar* var_ptr : var_ptrs) {
    JoinExprMap::iterator join_expr_iter = join_expr_map.find(var_ptr);
    internal_join(var_ptr, to_ite_expr_ptr(join_expr_iter));
  }
}

void Loop::internal_join(AbstractVar* var_ptr, IfThenElseExpr* join_expr) {
  JoinExprMap::iterator current_join_expr_iter = current_join_expr_map.find(var_ptr);
  if (current_join_expr_iter == current_join_expr_map.end()) {
    join_expr->set_then_expr(var_ptr->expr());
  } else {
    IfThenElseExpr* current_join_expr = to_ite_expr_ptr(current_join_expr_iter);
    current_join_expr->set_then_expr(var_ptr->expr());
  }

  var_ptr->set_expr(SharedExpr(join_expr));
}

inline IfThenElseExpr* Loop::to_ite_expr_ptr(const JoinExprMap::iterator& iter) {
  return (*iter).second;
}

}

