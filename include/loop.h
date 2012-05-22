#ifndef LOOP_H_
#define LOOP_H_

#include <map>
#include <stack>
#include <memory>
#include <unordered_set>

#include "var.h"
#include "reflect.h"

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

// A Loop object represents the unwinding of an iterative program structure
// such as do { ... } while( ... ) repetitions. On each iteration, the loop
// condition can be either true or false. However, since the condition can
// be over symbolic variables, both outcomes are often possible. Thus, each
// loop unwinding must generally account for both possibilities. For this
// purpose, a loop has member functions that can store and restore previous
// versions of symbolic variables. Unless these various versions are joined
// together, there could be an exponential number of these as the loop is
// unwound. For example, this occurs in a loop that has an if-then-else
// statement in which both branches are always possible on each iteration.
// Thus, a loop object has also a join operator. Joins are expected to be
// performed after every loop programming statement.
//
// The exact loop unwinding policy is controlled by dependency injection.
class Loop {

private:

  // Interface to injected loop unwinding policy implementation. Since this
  // policy could have state that is uniquely tied to the loop, this object
  // has the sole ownership of the pointer.
  std::unique_ptr<UnwindingPolicy> unwinding_policy_ptr;

  // cond_expr_stack is a stack of loop conditions. The top of the stack is the
  // loop condition of the inner-most loop unwinding.
  std::stack<SharedExpr> cond_expr_stack;

  // begin_expr_map is used to build the "else" branch of join expressions.
  // The key is the unique identifier of a symbolic variable. The value is
  // a stack of symbolic expressions where the top corresponds to the symbolic
  // expression that the variable had in the inner-most loop unwinding.
  std::map<const GenericVar* const, std::stack<SharedExpr>> begin_expr_map;

  // var_ptrs is a set of pointers to tracked variables. These are symbolic
  // variables whose symbolic expressions are going to be joined at the last
  // loop unwinding.
  std::unordered_set<GenericVar*> var_ptrs;

  // unwind_flag is true if and only if unwind(const Value<bool>&) should
  // delegate to the injected unwinding policy object. Initially, it is true.
  bool unwind_flag;

  // internal_stash(const GenericVar* const) pushes the current symbolic
  // expression of the supplied variable pointer on a stack. This stack
  // is unique to the variable pointer.
  void internal_stash(const GenericVar* const var_ptr);

  // internal_get_stack(const GenericVar* const var_ptr) gets all the stashed
  // expressions of the supplied variable pointer. The top of the stack is the
  // most recent expression that has been saved by internal_stash(var_ptr).
  std::stack<SharedExpr> internal_get_stack(const GenericVar* const var_ptr);

  // internal_unwind(const Value<bool>&) must be called if and only if the
  // unwinding policy allows another unwinding for the given boolean condition
  // and the previous loop unwindings.
  void internal_unwind(const Value<bool>& cond);

  // internal_join() must be called if and only if the unwinding policy forbids
  // another unwinding and internal_unwind(const Value<bool>&) has been called
  // at least once. internal_join() must call internal_join(GenericVar*) for
  // every pointer of a tracked variable. It may perform other sanity checks.
  void internal_join();

  // Creates a ternary join expression for a tracked variable.
  void internal_join(GenericVar* const var_ptr);

public:

  // Constructor which instantiates a BoundedUnwindingPolicy with the specified
  // maximum number of loop unwindings.
  Loop(const unsigned int k) :
    unwinding_policy_ptr(new BoundedUnwindingPolicy(k)),
    cond_expr_stack(), begin_expr_map(), var_ptrs(), unwind_flag(true) {}

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
    cond_expr_stack(), begin_expr_map(), var_ptrs(), unwind_flag(true) {}

  ~Loop() {}

  // unwind(const Value<bool>&) unwinds the loop once more if and only if
  // it returns true. The unwinding semantics is determined by the injected
  // unwinding policy implementation.
  bool unwind(const Value<bool>& cond);

  // track(Var<T>&) must be called before unwind(const Value<bool>&) is called
  // on each loop iteration. The argument is a variable that might be modified
  // in one loop iteration. It is safe to call track(Var<T>&) multiple times on
  // the same variable object. The order of calls does not matter.
  template<typename T>
  void track(Var<T>&);

};

template<typename T>
void Loop::track(Var<T>& var) { var_ptrs.insert(&var); }

#endif /* LOOP_H_ */
