#ifndef LOOP_H_
#define LOOP_H_

#include <map>
#include <stack>
#include <memory>
#include <stdexcept>

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
  std::map<uintptr_t, std::stack<SharedExpr>> begin_expr_map;

  // get_version_stack() is an internal function to get the expressions of a
  // variable. The top of the stack is the most recent expression that has
  // been saved by begin_loop(const Var<T>&) for the same variable.
  template<typename T>
  std::stack<SharedExpr> get_version_stack(const Var<T>& var) {
    return begin_expr_map[var.get_id()];
  }

  // put_version(const Var<T>&) pushes the current symbolic expression of
  // the supplied variable on a stack. This stack is unique to the variable.
  template<typename T>
  void put_version(const Var<T>& var) {
    begin_expr_map[var.get_id()].push(var.get_reflect_value().get_expr());
  }

public:

  // Constructor which instantiates a BoundedUnwindingPolicy with the specified
  // maximum number of loop unwindings.
  Loop(const unsigned int k) :
    unwinding_policy_ptr(new BoundedUnwindingPolicy(k)),
    cond_expr_stack(), begin_expr_map() {}

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
    cond_expr_stack(), begin_expr_map() {}

  ~Loop() {}

  // unwind(const Value<bool>&) unwinds the loop once more if and only if
  // it returns true. The unwinding semantics is determined by the injected
  // unwinding policy implementation.
  bool unwind(const Value<bool>& condition) {
    bool continue_flag = unwinding_policy_ptr->unwind(condition);
    if(continue_flag) {
      cond_expr_stack.push(condition.get_expr());
      return true;
    }

    return false;
  }

  // Must be called at the beginning of each loop iteration for each variable
  // that might be modified in one loop iteration.
  template<typename T>
  void begin_loop(const Var<T>&);

  // Must be called at the end of each loop iteration. Only arguments that were
  // specified to the begin_loop(const Var<T>&) member function are allowed.
  template<typename T>
  void end_loop(const Var<T>&);

  // Must be called after the loop has finished. The argument must be a symbolic
  // variable for which begin_loop(const Var<T>&) and end_loop(const Var<T>&)
  // has been called at the beginning and end of each loop iteration.
  //
  // Throws std::logic_error if any loop API misuses have been detected.
  template<typename T>
  void join(Var<T>&) throw(std::logic_error);


  // Remark: It would be nice if we could simplify the API to begin_loop() and
  // end_loop() without the need for the join() call after the loop programming
  // statement. This would mean that the join operation would have to occur in
  // end_loop(). Unfortunately, this implies that the symbolic expression for
  // the loop condition would generally consist of a join expression such as
  // "(([I]<5?([I]+1):[I])<5)" which is incorrect.
};

template<typename T>
void Loop::begin_loop(const Var<T>& var) { put_version(var); }

template<typename T>
void Loop::end_loop(const Var<T>& var) { /* for future use */}

template<typename T>
void Loop::join(Var<T>& var) throw(std::logic_error) {
  // TODO: for clarity we join only one variable at a time. If this turns out to
  // be too expensive, we can optimize by writing a single loop such that no
  // copy of the stack needs to be made.
  std::stack<SharedExpr> cond_expr_stack_copy(cond_expr_stack);
  std::stack<SharedExpr> version_stack = get_version_stack(var);
  if(cond_expr_stack_copy.empty() && version_stack.empty()) {
    // loop fell through so nothing needs to be done
    return;
  }

  if(version_stack.size() > cond_expr_stack_copy.size()) {
    throw std::logic_error("fewer unwind() calls than begin_loop() calls.");
  }

  if(version_stack.size() < cond_expr_stack_copy.size()) {
    throw std::logic_error("more unwind() calls than begin_loop() calls.");
  }

  // Construct If-Then-Else expression in reverse order of loop unwindings.
  // That is, the If-Then-Else expression of the last loop unwinding is
  // constructed first etc. Thus, pop cond_expr_stack_copy to get the inner-most
  // loop condition. Then, use the most recent version of the given symbolic
  // variable before and after the loop unwinding with which the popped
  // condition is associated. The "before" version corresponds to the "else"
  // branch of the new ternary expression whereas the "after" version is
  // assigned to its "then" branch.
  SharedExpr join_expr = var.get_reflect_value().get_expr();

  SharedExpr cond_expr;
  SharedExpr before_expr;
  do {

    cond_expr = cond_expr_stack_copy.top();
    before_expr = version_stack.top();

    join_expr = SharedExpr(new TernaryExpr(cond_expr, join_expr, before_expr));

    cond_expr_stack_copy.pop();
    version_stack.pop();

    // stop if either of both stacks is empty
  } while(!(cond_expr_stack_copy.empty() || version_stack.empty()));

  var.value.set_expr(join_expr);

  // Once the above optimization is implemented this goes away.
  if(begin_expr_map.empty()) {
    cond_expr_stack = std::stack<SharedExpr>();
  }
}

#endif /* LOOP_H_ */
