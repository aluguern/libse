// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LOOP_H_
#define LOOP_H_

#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "var.h"
#include "value.h"

namespace se {

/*! \file loop.h */

/// Unwinding semantics of a loop
  
/// Such a policy is said to be sound if and only if every found bug does in
/// fact exist. If a policy is not sound, we call it unsound. A policy is said
/// to be complete if and only if it it finds all existing bugs. Otherwise, it
/// is said to be incomplete. Due to Turing's halting problem, there exists no
/// policy that is both sound and complete.
class UnwindingPolicy {

public:

  /// Should loop be unwound at least once more?
  virtual bool unwind(const Value<bool>&) = 0;

  // Every interface needs a virtual destructor.
  virtual ~UnwindingPolicy() {}

};

/// Bounded loop unwinding semantics

/// This policy is sound relative to the SMT/SAT solver but incomplete.
class BoundedUnwindingPolicy : public UnwindingPolicy {

private:

  // k is the maximum number of loop unwindings.
  const unsigned int k;

  // j is the number of loop unwindings. It is initialized to zero.
  unsigned int j;

public:

  /// Specifies the maximum number of loop unwindings
  BoundedUnwindingPolicy(const unsigned int k) : k(k), j(0) {}
  ~BoundedUnwindingPolicy() {}

  /// Has unwind(const Value<bool>&) been called strictly less than k times?

  /// Note that the argument is always ignored.
  bool unwind(const Value<bool>& ignore) {
    // Since "j++; return j < k;" could result in overflow we require that
    // j be strictly less than k before incrementing the counter.
    if(j < k) { j++; return true; }
    return false;
  }

};

/// \brief Control-flow annotation for multi-path symbolic execution of
/// a loop statement

/// Allows the symbolic execution of an iterative program control-flow
/// statement such as a `do { ... } while( ... )` loop. Both concrete and
/// symbolic loop conditions are supported. A concrete condition can be only
/// either true or false. In contrast, a symbolic loop condition has no
/// definite truth value since it references symbolic variables which range
/// over arbitrary values. Thus, each loop unwinding must generally account for
/// the possibility that the loop exists or continues. Thus, there could be an
/// exponential number of execution paths that would need to be considered as
/// the loop is unwound. To see this, consider a loop that has an if-then-else
/// statement with a symbolic branch condition such that both branches are
/// possible on each loop iteration.
///
/// Single-path symbolic execution reasons about these paths on an individual
/// basis. This tends to be tedious and slow. Therefore, this API embraces the
/// multi-path symbolic execution strategy which encodes multiple paths as a
/// symbolic expression. This is called path joining.
///
/// The exact loop unwinding policy should be configurable through dependency
/// injection. Such a policy determines when the unwinding of a loop terminates.
class Loop {

private:

  // Hash table whose search and insertion operations must have at least
  // average constant-time complexity. Unfortunately, constness must be
  // dropped because the <functional> header only defines template<class T>
  // struct hash<T*>. For this reason, the constness of function arguments
  // is only documented with comments for now.
  typedef std::unordered_map<AbstractVar*, IfThenElseExpr*> JoinExprMap;

  // Strictly internal constant for an unknown expression
  static const SharedExpr NIL_EXPR;

  // Interface to injected loop unwinding policy implementation. Since this
  // policy could have state that is uniquely tied to the loop, this object
  // has the sole ownership of the pointer.
  std::unique_ptr<UnwindingPolicy> unwinding_policy_ptr;

  // Set of pointers of variables whose symbolic expressions are joined at
  // the last loop unwinding.
  std::unordered_set<AbstractVar*> var_ptrs;

  // Maps each AbstractVar pointer to the root of a DAG which represents the
  // join expression of the variable once the loop has been fully unwound.
  // The map is initialized during the first loop unwinding. After this
  // initialization, it can be read but never be written again.
  JoinExprMap join_expr_map;

  // Maps the physical address of an AbstractVar to a pointer of a IfThenElseExpr
  // object. It represents a join expression of the form "x ? y : z" where x,
  // y and z are symbolic expressions. When such a new join expression is
  // inserted into the map, x corresponds to the loop condition of the current
  // loop unwinding. Initially, y is NULL whereas z is the current symbolic
  // expression of the symbolic variable whose physical memory address serves
  // as key. With the next loop unwinding, y is updated to the join expression
  // for this new loop unwinding.
  //
  // Note that when an expression is inserted into the map for the first time,
  // it must be associated with the "then" expression of the join expression
  // in join_expr_map for the same key.
  JoinExprMap current_join_expr_map;

  // Allow the unwinding policy another loop unwinding given the most recent
  // loop condition?
  bool unwind_flag;

  // Must be called exactly once before any of the internal data structures
  // are modified.
  void internal_init(const Value<bool>& cond);

  // Must be called if and only if the unwinding policy allows another
  // unwinding for the given boolean condition and join_expr_map has been
  // initialized. If this precondition is satisfied, then
  // internal_current_join(const Value<bool>&, const AbstractVar* const) is
  // called for the supplied loop condition and each pointer in var_ptrs.
  void internal_unwind(const Value<bool>& cond);

  // Must only read and write the current_join_expr_map field whose class
  // invariants are documented as part of its declaration.
  void internal_current_join(const Value<bool>& cond, AbstractVar* var_ptr);

  // Must be called if and only if the unwinding policy forbids another
  // unwinding and unwind(const Value<bool>&) has been called at least once.
  // If this precondition has been satisfied, internal_join() calls the
  // function internal_join(AbstractVar* const, IfThenElseExpr*) for every tracked
  // variable pointer and the pointer of the join expression with which it
  // should be associated.
  void internal_join();

  // Must create the strongest postcondition for the given variable after the
  // loop has been fully unwound.
  //
  // Note: the function assumes that at least one loop unwinding has occurred.
  //
  // For the computation of the strongest postcondition, two cases must be
  // considered:
  //
  // 1) The loop has been unwound exactly once
  // 2) The loop has been unwound more than once
  //
  // The required behaviour in each case is determined by the postcondition
  // of internal_current_join(const Value<bool>&, const AbstractVar* const).
  //
  // Finally, the symbolic expression of the pointed to variable must be set
  // to the computed strongest postcondition.
  void internal_join(AbstractVar* var_ptr, IfThenElseExpr* join_expr);

  // Internal helper function that extracts the join expression the iterator.
  static IfThenElseExpr* to_ite_expr_ptr(const Loop::JoinExprMap::iterator& iter);

  Loop(const Loop&) = delete;
  Loop& operator=(const Loop&) = delete; 

public:

  /// Bounded loop

  /// Symbolic execution of a control-flow structure whose repetition is bound
  /// by the specified maximum number of loop unwindings.
  Loop(const unsigned int k) :
    unwinding_policy_ptr(new BoundedUnwindingPolicy(k)),
    join_expr_map(), current_join_expr_map(), var_ptrs(), unwind_flag(true) {}

  /// Loop with configurable loop unwinding policy

  /// The ownership of the pointer to the policy object must be transferred to
  /// the newly constructed loop object! Since you cannot copy a unique_ptr,
  /// you can only move it. The proper way to do this is with the std::move
  /// standard library function.
  ///
  /// Example:
  ///
  ///     std::unique_ptr<UnwindingPolicy> ptr(new BoundedUnwindingPolicy(k));
  ///     Loop loop(std::move(ptr));
  ///
  /// See also Nicol Bolas' comments on Stackoverflow to learn more about
  /// how to "pass a unique_ptr argument to a constructor or a function".
  Loop(std::unique_ptr<UnwindingPolicy> policy_ptr) :
    unwinding_policy_ptr(std::move(policy_ptr)),
    join_expr_map(), current_join_expr_map(), var_ptrs(), unwind_flag(true) {}

  ~Loop() {}

  /// Unwind the loop once more if the loop unwinding policy permits it

  /// The unwinding semantics is determined by the injected unwinding policy
  /// implementation. If the return value is false, the effect of subsequent
  /// unwind(const Value<bool>&) calls is undefined.
  ///
  /// Subclasses may perform extra work on each unwinding.
  /// \see track(Var<T>&)
  virtual bool unwind(const Value<bool>& cond);

  /// Allow the given variable to be modified in the loop

  /// Must be called before unwind(const Value<bool>&) is first called. The
  /// argument is a variable that might be modified in zero or more loop
  /// iterations. It is safe to call track(Var<T>&) multiple times on the
  /// same variable object. The order of these calls does not matter.
  template<typename T>
  void track(Var<T>& var) { var_ptrs.insert(&var); }

};

}

#endif /* LOOP_H_ */
