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

/*! \file if.h */

/// \brief Control-flow annotation for multi-path symbolic execution of
/// an if-then-else statement

/// An If object can build symbolic expressions for variables whose
/// modifications are subject to a symbolic Boolean condition. The required
/// annotation of an if-then-else statement is illustrated below.
///
/// Example:
/// ~~~{.cpp}
/// int j = *;
///
/// if (j < 0) {
///   j = 0;
/// } else {
///   j = 1;
/// }
/// ~~~
/// should be rewritten as
/// ~~~{.cpp}
/// se::Int j = se::any<int>("J");
///
/// se::If branch(j < 0);
/// branch.track(j);
/// if (branch.begin_then()) { j = 0; }
/// if (branch.begin_else()) { j = 1; }
/// branch.end();
/// ~~~
///
/// Note that this source-to-source transformation requires the immediate
/// post-dominator of every control point in the program's control-flow graph
/// to be always computable. For example, this is not the case in unstructured
/// programs where goto statements can jump to an arbitrary program location.
class If {

private:

  // Hashtable whose search and insertion operations must have at least average
  // constant-time complexity. Unfortunately, constness must be dropped because
  // the <functional> header only defines template<class T> struct hash<T*>.
  // For this reason, the constness of function arguments is only documented
  // with comments for now.
  typedef std::unordered_map<AbstractVar*, IfThenElseExpr*> JoinExprMap;

  // Strictly internal constant for an unknown expression.
  static const SharedExpr NIL_EXPR;

  // Set of pointers of variables whose symbolic expressions must be joined
  // when reaching the end of the if-then-else statement.
  std::unordered_set<AbstractVar*> var_ptrs;

  // Map each AbstractVar pointer to the root of a DAG that represents the
  // join expression of the variable once the loop has been fully unwound.
  // The map is initialized during the first loop unwinding. After this
  // initialization, it can be read but never be written again.
  JoinExprMap join_expr_map;

  // flag is true if and only if the control statement has an "else" block.
  bool if_then_else;

  // Find pointer to the ternary expression with which the symbolic variable
  // pointer has been associated through the track(Var<T>&) API.
  IfThenElseExpr* find_join_expr_ptr(AbstractVar*);

  If(const If&);
  If& operator=(const If&);

protected:

  /// Branch condition

  /// Boolean value which represents the symbolic or concrete condition that
  /// guards the "then" and  "else" block of the control-flow statement. The
  /// condition has a symbolic expression if and only if \ref is_symbolic_cond
  /// is true.
  const Value<bool> cond;

  /// Is branch condition symbolic?

  /// flag is true if and only if @ref cond has a symbolic expression.
  const bool is_symbolic_cond;

  /// Has if statement an "else" block?
  bool is_if_then_else() const { return if_then_else; }

public:

  /// Create an if-then-else statement with the given Boolean condition.
  If(const Value<bool>& cond) :
    cond(cond), is_symbolic_cond(cond.is_symbolic()),
    if_then_else(false), join_expr_map(), var_ptrs() {}

  ~If() {}

  /// Begin the "then" branch

  /// begin_then() denotes the beginning of the "then" block. This block is
  /// ignored if and only if the return value is false.
  ///
  /// This member function must be called exactly once prior to calling
  /// begin_else() or end().
  ///
  /// Subclasses may perform extra work based on the if statement condition.
  virtual bool begin_then();

  /// Begin the "else" branch

  /// Must annotate the end of the "then" block and the beginning of the "else"
  /// block. Therefore, begin_else() must never be called prior to calling
  /// begin_then(). In addition, this member function should be called at most
  /// once. The "else" block is ignored if and only if false is returned.
  ///
  /// Subclasses may perform extra work based on the if statement condition.
  virtual bool begin_else();

  /// End if-then-else statement

  /// end() must always be called exactly once such that its call location is
  /// the immediate post-dominator of the if-then-else statement.
  ///
  /// Subclasses may perform extra work based on the if statement condition.
  virtual void end();

  /// Allow the given variable to be modified in a guarded block

  /// Must be called if and only if no other member function has been invoked
  /// yet. The procedure's argument is a symbolic variable whose value might
  /// be modified in either the "then" or "else" block, or both. It is safe to
  /// call track(Var<T>&) multiple times on the same variable. Also the order
  /// of these calls does not matter.
  template<typename T>
  void track(Var<T>& var) { var_ptrs.insert(&var); }

};

}

#endif /* IF_H_ */
