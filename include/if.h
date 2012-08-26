// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef IF_H_
#define IF_H_

#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "var.h"
#include "value.h"

namespace se {

// An If object can build the symbolic expression of variables whose
// modifications are subject to a Boolean condition. A typical usage
// of an If class is the symbolic execution of an if-then-else statement.
// Even though the "else" block can be optional, the control-flow statement
// must always have a computable immediate post-dominator.
class If {

private:

  // JoinExprMap is an abstract data type for a hashtable whose search and
  // insertion operations must have at least average constant-time complexity.
  // Unfortunately, constness must be dropped because the <functional> header
  // only defines template<class T> struct hash<T*>. For this reason, the
  // constness of function arguments is only documented with comments for now.
  typedef std::unordered_map<GenericVar*, TernaryExpr*> JoinExprMap;

  // NIL_EXPR is a strictly internal constant for an unknown expression.
  static const SharedExpr NIL_EXPR;

  // var_ptrs is a set of pointers of tracked variables whose symbolic
  // expressions must be joined when reaching the end of the if statement.
  std::unordered_set<GenericVar*> var_ptrs;

  // join_expr_map maps each GenericVar pointer to the root of a DAG that
  // represents the join expression of the variable once the loop has been
  // fully unwound. The map is initialized during the first loop unwinding.
  // After this initialization, it can be read but never be written again.
  JoinExprMap join_expr_map;

  // flag is true if and only if the if statement has an "else" block.
  bool is_if_then_else;

  // find_join_expr_ptr(const GenericVar* const) returns a pointer to the
  // ternary expression with which the symbolic variable pointer has been
  // associated through the track(Var<T>&) API.
  TernaryExpr* find_join_expr_ptr(GenericVar*);

  // Boolean value which represents the condition that guards the "then" and
  // "else" block of the control-flow statement. The condition has a symbolic
  // expression if and only if the is_symbolic_cond flag is true.
  const Value<bool> cond;

  // flag that indicates if the condition value has a symbolic expression.
  const bool is_symbolic_cond;

public:

  // Constructor which can be used to build symbolic expressions of variables
  // whose values are being modified in an if-then-else control-flow statement.
  If(const Value<bool>& cond) :
    cond(cond), is_symbolic_cond(cond.is_symbolic()),
    is_if_then_else(false), join_expr_map(), var_ptrs() {}

  ~If() {}

  
  // begin_then() denotes the beginning of the "then" block. Note that this
  // block is ignored if and only if the return value is false. This member
  // function must always be called exactly once.
  bool begin_then();
  
  // begin_else() denotes the end of the "then" block and the beginning of
  // the "else" block. For this reason, execute_else() must never be called
  // prior to calling execute_then(). In addition, this member function should
  // be called at most once. Note that the "else" block is ignored if and only
  // if the return value is false.
  bool begin_else();

  // end() must always be called exactly once such that its call location is the
  // immediate post-dominator of the if-then-else statement.
  void end();

  // track(Var<T>&) must be called if and only if no other member function has
  // been invoked yet. The procedure's argument is a symbolic variable whose
  // value might be modified in either the "then" or "else" block, or both. It
  // is safe to call track(Var<T>&) multiple times on the same variable object.
  // Also the order of calls does not matter.
  template<typename T>
  void track(Var<T>& var) { var_ptrs.insert(&var); }

};

}

#endif /* IF_H_ */
