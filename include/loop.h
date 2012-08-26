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

// UnwindingPolicy is an interface that specifies the unwinding semantics of a
// loop. Such a policy is said to be sound if and only if every found bug with
// it is in fact real. If a policy is not sound, we call it unsound. A policy
// is called complete if and only if an analysis never fails to find bugs with
// it. Otherwise, it is said to be incomplete. Due to Turing's halting problem,
// there exists no policy that is both sound and complete.
class UnwindingPolicy {

public:

  // unwind(const Value<bool>&) returns true if and only if the loop should be
  // unwound at least once more.
  virtual bool unwind(const Value<bool>&) = 0;

  // Every interface needs a virtual destructor.
  virtual ~UnwindingPolicy() {}

};

// BoundedUnwindingPolicy implements a loop unwinding policy which enforces a
// maximum number of a loop iterations. This policy is sound but incomplete.
class BoundedUnwindingPolicy : public UnwindingPolicy {

private:

  // k is the maximum number of loop unwindings.
  const unsigned int k;

  // j is the number of loop unwindings. It is initialized to zero.
  unsigned int j;

public:

  // Constructor which specified the maximum number of loop unwindings.
  BoundedUnwindingPolicy(const unsigned int k) : k(k), j(0) {}
  ~BoundedUnwindingPolicy() {}

  // unwind(const Value<bool>&) returns true if and only if it the function has
  // been called strictly less than k times. Thus, the argument is ignored.
  bool unwind(const Value<bool>& ignore) {
    // Since "j++; return j < k;" could result in overflow we require that
    // j be strictly less than k before incrementing the counter.
    if(j < k) { j++; return true; }
    return false;
  }

};

// Loop can be used to control and capture the execution of an iterative
// program control-flow statement such as a do { ... } while( ... ) loop. Both
// concrete and symbolic loop conditions are supported. A concrete condition
// can be only either true or false. In contrast, a symbolic loop condition has
// no definite truth value since it references symbolic variables which range
// over arbitrary values. Thus, each loop unwinding must generally account for
// the possibility that the loop exists or continues. Thus, there could be an
// exponential number of execution paths that would need to be considered as
// the loop is unwound. To see this, consider a loop that has an if-then-else
// statement with a symbolic branch condition such that both branches are
// possible on each loop iteration.
//
// Single-path symbolic execution reasons about these paths on an individual
// basis. This tends to be tedious and slow. Therefore, this API embraces the
// multi-path symbolic execution strategy which encodes multiple paths as a
// symbolic expression. This is called path joining.
//
// The exact loop unwinding policy should be configurable through dependency
// injection. Such a policy determines when the unwinding of a loop terminates.
class Loop {

private:

  // JoinExprMap is an abstract data type for a hashtable whose search and
  // insertion operations must have at least average constant-time complexity.
  // Unfortunately, constness must be dropped because the <functional> header
  // only defines template<class T> struct hash<T*>. For this reason, the
  // constness of function arguments is only documented with comments for now.
  typedef std::unordered_map<GenericVar*, TernaryExpr*> JoinExprMap;

  // NIL_EXPR is a strictly internal constant for an unknown expression.
  static const SharedExpr NIL_EXPR;

  // Interface to injected loop unwinding policy implementation. Since this
  // policy could have state that is uniquely tied to the loop, this object
  // has the sole ownership of the pointer.
  std::unique_ptr<UnwindingPolicy> unwinding_policy_ptr;

  // var_ptrs is a set of pointers of tracked variables whose symbolic
  // expressions must be joined at the last loop unwinding.
  std::unordered_set<GenericVar*> var_ptrs;

  // join_expr_map maps each GenericVar pointer to the root of a DAG that
  // represents the join expression of the variable once the loop has been
  // fully unwound. The map is initialized during the first loop unwinding.
  // After this initialization, it can be read but never be written again.
  JoinExprMap join_expr_map;

  // current_join_expr_map maps the physical address of a GenericVar to a
  // pointer of a TernaryExpr object. It represents a join expression of the
  // form "x ? y : z" where x, y and z are symbolic expressions. When such a
  // new join expression is inserted into the map, x corresponds to the loop
  // condition of the current loop unwinding. Initially, y is NULL whereas z
  // is the current symbolic expression of the symbolic variable whose physical
  // memory address serves as key. With the next loop unwinding, y is updated
  // to the join expression for this new loop unwinding.
  //
  // Note that when an expression is inserted into the map for the first time,
  // it must be associated with the "then" expression of the join expression
  // in join_expr_map for the same key.
  JoinExprMap current_join_expr_map;

  // unwind_flag is true if and only if the unwinding policy allows another
  // loop unwinding given the most recent loop condition.
  bool unwind_flag;

  // internal_init(const Value<bool>&) must be called only once before any of
  // the internal data structures are modified.
  void internal_init(const Value<bool>& cond);

  // internal_unwind(const Value<bool>&) must be called if and only if the
  // unwinding policy allows another unwinding for the given boolean condition
  // and join_expr_map has been initialized. If this precondition is satisfied,
  // internal_current_join(const Value<bool>&, const GenericVar* const) is
  // called for the supplied loop condition and each pointer in var_ptrs.
  void internal_unwind(const Value<bool>& cond);

  // internal_current_join(const Value<bool>&, const GenericVar* const) must
  // only read and write the current_join_expr_map field whose class invariants
  // are documented as part of its declaration.
  void internal_current_join(const Value<bool>& cond, GenericVar* var_ptr);

  // internal_join() must be called if and only if the unwinding policy forbids
  // another unwinding and unwind(const Value<bool>&) has been called at least
  // once. If this precondition has been satisfied, internal_join() calls the
  // function internal_join(GenericVar* const, TernaryExpr*) for every pointer
  // of a tracked variable and the pointer of the join expression with which it
  // should be associated.
  void internal_join();

  // internal_join(GenericVar* const, TernaryExpr*) must create the strongest
  // postcondition for the given variable after the loop has been fully unwound.
  // Note: the function assumes that at least one loop unwinding has occurred.
  //
  // For the computation of the strongest postcondition, two cases must be
  // considered:
  //
  // 1) The loop has been unwound exactly once
  // 2) The loop has been unwound more than once
  //
  // The required behaviour in each case is determined by the postcondition
  // of internal_current_join(const Value<bool>&, const GenericVar* const).
  //
  // Finally, the symbolic expression of the pointed to variable must be set
  // to the computed strongest postcondition.
  void internal_join(GenericVar* var_ptr, TernaryExpr* join_expr);

  // to_ternary_expr_ptr(const JoinExprMap::iterator&) is an internal helper
  // function that extracts the join expression from an JoinExprMap iterator.
  static TernaryExpr* to_ternary_expr_ptr(const Loop::JoinExprMap::iterator& iter);

public:

  // Constructor which instantiates a BoundedUnwindingPolicy with the specified
  // maximum number of loop unwindings.
  Loop(const unsigned int k) :
    unwinding_policy_ptr(new BoundedUnwindingPolicy(k)),
    join_expr_map(), current_join_expr_map(), var_ptrs(), unwind_flag(true) {}

  // Constructor using dependency injection to set the loop unwinding policy.
  // The ownership of the pointer to the policy object must transferred to the
  // newly constructed loop object! Since you cannot copy a unique_ptr, you can
  // only move it. The proper way to do this is with the std::move standard
  // library function.
  //
  // Example:
  //
  //   std::unique_ptr<UnwindingPolicy> ptr(new BoundedUnwindingPolicy(k));
  //   Loop loop(std::move(ptr));
  //
  // See also Nicol Bolas' comments on Stackoverflow to learn more about
  // how to "pass a unique_ptr argument to a constructor or a function".
  Loop(std::unique_ptr<UnwindingPolicy> policy_ptr) :
    unwinding_policy_ptr(std::move(policy_ptr)),
    join_expr_map(), current_join_expr_map(), var_ptrs(), unwind_flag(true) {}

  ~Loop() {}

  // unwind(const Value<bool>&) unwinds the loop once more if and only if
  // it returns true. The unwinding semantics is determined by the injected
  // unwinding policy implementation. If the return value is false, the effect
  // of subsequent unwind(const Value<bool>&) calls is undefined.
  bool unwind(const Value<bool>& cond);

  // track(Var<T>&) must be called before unwind(const Value<bool>&) is called
  // on each loop iteration. The argument is a variable that might be modified
  // in one loop iteration. It is safe to call track(Var<T>&) multiple times on
  // the same variable object. The order of calls does not matter.
  template<typename T>
  void track(Var<T>& var) { var_ptrs.insert(&var); }

};

}

#endif /* LOOP_H_ */
