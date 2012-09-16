// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "if.h"

namespace se {

const SharedExpr If::NIL_EXPR = SharedExpr(0);

bool If::begin_then() {
  if(is_symbolic_cond) {
    const SharedExpr& cond_expr = cond.expr();
    for(AbstractVar* var_ptr : var_ptrs) {
      IfThenElseExpr* join_expr = new IfThenElseExpr(cond_expr, NIL_EXPR, var_ptr->expr());
      join_expr_map.insert(std::make_pair(var_ptr, join_expr));
      var_ptr->stash();
    }
    return true;
  } else {
    return cond.value();
  }
}

bool If::begin_else() {
  if_then_else = true;
  assert(is_if_then_else());

  if(is_symbolic_cond) {
    const SharedExpr& cond_expr = cond.expr();
    for(AbstractVar* var_ptr : var_ptrs) {
      IfThenElseExpr* join_expr = find_join_expr_ptr(var_ptr);
      const SharedExpr& expr = var_ptr->expr();
      join_expr->set_then_expr(expr);
      var_ptr->unstash(true);
    }
    return true;
  } else {
    return !cond.value();
  }
}

void If::end() {
  if(!is_symbolic_cond) {
    // no joining needed
    return;
  }

  for(AbstractVar* var_ptr : var_ptrs) {
    IfThenElseExpr* join_expr = find_join_expr_ptr(var_ptr);
    const SharedExpr& expr = var_ptr->expr();
    if (is_if_then_else()) {
      join_expr->set_else_expr(expr);
    } else {
      var_ptr->unstash(false);
      join_expr->set_then_expr(expr);
    }

    // TODO: Support partial expressions
    // Has var_ptr been updated in at least the "then" or "else" block?
    if (join_expr->then_expr() != join_expr->else_expr()) {
      var_ptr->set_expr(SharedExpr(join_expr));
    }
  }
}

inline IfThenElseExpr* If::find_join_expr_ptr(AbstractVar* var_ptr) {
  const JoinExprMap::iterator& join_expr_iter = join_expr_map.find(var_ptr);
  assert(join_expr_iter != join_expr_map.end());
  return (*join_expr_iter).second;
}

}

