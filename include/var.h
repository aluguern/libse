// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef VAR_H_
#define VAR_H_

#include <stack>
#include <string>
#include <sstream>
#include <cstdint>

#include "expr.h"
#include "value.h"
#include "tracer.h"

namespace se {

/*! \file var.h */

static const std::string SymbolicVarPrefix = "Var_";

/// Built-in unsigned numerical type which can be initialized to \ref VZERO.
typedef unsigned long long Version;

/// Zero according to \ref Version type.
const Version VZERO = 0LL;

/// Base class for a (symbolic/concrete) lvalue

/// Base class for an lvalue with optional concrete data and an optional
/// symbolic expression.
/// 
/// More precisely, the class can associate a C++ lvalue (locator value) with
/// an algebraic expression in a certain theory. In the previous example, a
/// C++ statement which increments a the symbolic variable `x` by three would
/// could yield the symbolic expression "x + 3" in the ring of integers.
/// By choosing an appropriate theory, the API can be used to successively
/// approximate the strongest postcondition of a program variable. This is
/// typically referred to as symbolic execution. The variable is suitable for
/// multi-path symbolic execution only if if it is_symbolic(). In addition,
/// if it is_concrete(), then it can be used in single-path symbolic execution.
///
/// Therefore, an AbstractVar object's symbolic expression could be seen as a
/// function on the program inputs. Thus, an assignment never irrevocably
/// overwrites any information. This contrasts conventional assignments in
/// imperative programming languages. For this reason, the API could be also
/// useful for debugging purposes.
///
/// Changes to the variable are tracked by a version number. The version number
/// is always initialized to zero. Subsequently, on each modifier operation, it
/// is incremented by one. These version numbers are never based on the version
/// information of another AbstractVar object. This could simplify the use of
/// the generating of dynamic single assignment (DSA) forms.
///
/// Except the version information, the internal state of a variable can be
/// saved and restored with stash() and \ref unstash(bool) "unstash(true)",
/// respectively. It is a programmer error to call unstash(bool) more times
/// than stash(). To conserve resources, every stash() call should be matched
/// with a corresponding unstash(bool) call (note that the flag value is irrelevant
/// for resource deallocation purposes).
///
/// Finally, each implementation of the AbstractVar interface must take type
/// casting into account. These type casts should be explicit in a \ref expr()
/// "variable's symbolic expression"; the proper way of doing this is through
/// CastExpr.
class AbstractVar {

public:

  /// type information

  /// Note that any precision loss that might have occurred due to type casting
  /// is conservatively approximated by is_cast().
  ///
  /// \see AbstractValue::type()
  virtual Type type() const = 0;

  /// Is concrete or symbolic value subject to an up or down cast?

  /// If an unsafe type conversion is needed, then is_cast() returns true.
  /// Note that the converse is false since is_cast() also returns true for
  /// up casts.
  virtual bool is_cast() const = 0;

  /// Force variable to be symbolic

  /// The supplied name must uniquely identify the newly created symbolic
  /// variable among the program's inputs.
  ///
  /// \see AbstractValue::set_symbolic(const std::string&)
  virtual void set_symbolic(const std::string& identifier) = 0;

  /// Is symbolic expression defined?

  /// \see AbstractValue::is_symbolic()
  /// \see AbstractValue::expr()
  virtual bool is_symbolic() const = 0;

  /// Is concrete data defined?

  /// \see AbstractValue::is_concrete()
  virtual bool is_concrete() const = 0;

  /// Non-decreasing number which increments with every assignment

  /// How often has the variable been assigned a new value or a
  /// symbolic expression?
  virtual Version version() const = 0;

  /// Sets the symbolic expression of the variable

  /// Unless the argument is NULL, is_symbolic() returns true afterwards.
  /// This setter increments the version number by one each time it is called
  /// regardless of the supplied expression.
  ///
  /// \see AbstractValue::set_expr(const SharedExpr&)
  virtual void set_expr(const SharedExpr&) = 0;

  /// Symbolic expression of the variable

  /// The return value is a NULL pointer if and only if is_symbolic() is false.

  /// /see AbstractValue::expr()
  virtual const SharedExpr expr() const = 0;

  /// Save internal state

  /// All version information remains unaffected by this call.
  virtual void stash() = 0;

  /// Restore internal state (i.e. reverse stash())

  /// `unstash(true)` restores the most recently stashed internal variable state.
  /// It also deallocates all the resources which stored this internal state.
  /// The variable's version number is incremented by one if and only if it has
  /// been modified since the most recent stash() call.
  ///
  /// In contrast, `unstash(false)` only deallocates resources acquired by the
  /// most recent stash() call; the internal variable state remains unaffected.
  /// Thus, the versioning information never changes when the flag is false.
  ///
  /// Regardless of the flag value, it is a programmer error to call
  /// unstash(bool) more times than stash() has been invoked.
  virtual void unstash(bool) = 0;

  // Every interface needs a public virtual destructor.
  virtual ~AbstractVar() {}
};

/// Type-safe (symbolic/concrete) lvalue

/// Example:
///
///     Int x = any<int>("X");
///     Int y = 7;
///     Int z = x + 3;
///     y = z + 5;
///
/// Here `x` is said to be a \ref is_symbolic() "symbolic" variable whose
/// type() is \ref INT. In contrast, `y` is an integer variable with only
/// concrete data (e.g. 7). However, `y` can become symbolic later in the
/// symbolic execution of the program under test (e.g. last assignment). Due to
/// \ref Value::aggregate() "constant propagation", `y`'s final Var<T>::expr()
/// is of the form "x + 8".
///
/// The concrete data and/or symbolic expression of a variable can be accessed
/// through data(). See the AbstractVar and Value class documentation for more
/// details.
///
/// The following table summarizes common mechanisms to create a variable:
///
///   Code | Description 
/// ------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------
///  Var<T> x = \ref any(const std::string&) "any"<T>("X")                                     | \ref is_symbolic() "symbolic" variable of type `T` and identifier "X"
///  Var<T> x = [primitive value]                                                              | \ref is_concrete() "concrete" variable of type `T`
///  Var<T> x = [primitive value]; x.\ref set_symbolic(const std::string&) "set_symbolic"("X") | variable of type `T` which is \ref is_symbolic() "symbolic" and \ref is_concrete() "concrete"
///
/// Once a variable has been created, it can be used as any other regular
/// program variable as long as the required operators have been overloaded.
template<typename /* primitive type */T>
class Var : public AbstractVar {
 
private:

  // Concrete and/or symbolic value of type T
  Value<T> m_value;

  // true if and only if m_value's data is the result of an up or downcast.
  // If the variable is symbolic and cast is true, then its expression is
  // also going to have a CastExpr for the required type cast.
  bool m_cast;

  // Version number that never decreases; initially, it is VZERO. With each
  // assignment operation, the version number is incremented by one.
  Version m_version;

  // stack to stash() and unstash() internal state
  struct State { const Value<T> value; const bool cast; Version version; };
  std::stack<State> m_stack;

public:

  /// Concrete variable
  Var(const T data) : m_value(data), m_cast(false), m_version(VZERO) {}

  /// Variable based on another (symbolic/concrete) value of the same type
  Var(const Value<T>& value) : m_value(value), m_cast(false),
                               m_version(VZERO) {}

  /// Variable based on another (symbolic/concrete) value of a different type

  /// Since the type of the value differ from the template type T,
  /// the new variable's concrete data could be subject to precision loss;
  /// this is conservatively approximated by is_cast(). Moreover, if the
  /// value is_symbolic(), the symbolic expression of the variable is going
  /// to have a new CastExpr.
  template<typename S>
  Var(const Value<S>& value) : m_value(value), m_cast(true),
    m_version(VZERO) {}

  /// Safe copy constructor

  /// Copy constructor that instantiates a variable by creating a deep copy of
  /// the supplied variable's concrete data. Any symbolic expressions are
  /// shared between both the original and copied variable object until either
  /// one is modified. Note that casts are transitive: if other.is_cast() is
  /// true, then the copy's is_cast() is true.
  Var(const Var& other) : m_value(other.m_value), m_cast(other.m_cast),
                          m_version(VZERO) {}

  /// Unsafe copy constructor with type casting

  /// Copy conversion constructor that creates a copy of another variable.
  /// Incompatibilities between the type of the supplied variable's concrete
  /// data and the type of the instantiated variable object are resolved by
  /// implicit type casting. In the case in which the other variable is also
  /// symbolic, the new variable's expr() is going to have a new CastExpr.
  template<typename S>
  Var(const Var<S>& other) : m_value(other.value()), m_cast(true),
                             m_version(VZERO) {}

  ~Var() {}

  /// Concrete/symbolic value

  /// Read-only reference to an object that contains the concrete value and
  /// runtime information about this variable. The caller must ensure that
  /// it does not dereference the return value after this object has been
  /// destroyed.
  ///
  /// \see Value
  const Value<T>& value() const { return m_value; }

  /// Concrete data (if defined)

  /// The return value is defined if and only if is_concrete() is true.
  /// Note that if is_symbolic() is true, the conversion could render
  /// concolic execution incomplete. Also note that the bool() conversion
  /// operator adds the symbolic expression to the \ref tracer() "global"
  /// path constraints if and only if is_symbolic() returns true.
  ///
  /// If type casts coerce the data, is_cast() returns true.
  ///
  /// \see Value::operator T()
  operator T() const {
    if(is_symbolic()) {
       // TODO: log possibility of incomplete analysis
    }

    return m_value.data();
  }

  /// Replace the value and propagate cast information

  /// Copy any concrete data symbolic expression of the other variable. Any
  /// required type conversions are first performed by Var(const Var<S>&).
  /// Such a conversion could result in precision lost. This is conservatively
  /// approximated by is_cast().
  ///
  /// Note that when the variable is assigned to itself, no data is copied.
  /// Also self-assignments leave the version number unchanged.
  Var& operator=(const Var& other) {
    if (this != &other) {
      m_value = other.m_value;
      m_cast = other.m_cast;
      m_version++;
    }

    return *this;
  }

  Type type() const { return TypeConstructor<T>::type; }
  bool is_cast() const { return m_cast; }
  void set_symbolic(const std::string& identifier) {
    m_value.set_symbolic(identifier);
  }
  bool is_symbolic() const { return m_value.is_symbolic(); }
  bool is_concrete() const { return m_value.is_concrete(); }
  Version version() const { return m_version; }
  const SharedExpr expr() const { return m_value.expr(); }
  void set_expr(const SharedExpr& expr) {
    m_version++;
    m_value.set_expr(expr);
  }

  void stash() { m_stack.push(State{ m_value, m_cast, m_version }); }
  void unstash(bool restore) {
    if (restore) {
      const State& state = m_stack.top();
      if (m_version != state.version) {
        m_cast = state.cast;
        m_value = state.value;
        m_version++;
      }
    }

    m_stack.pop();
  }
};

typedef Var<bool> Bool;
typedef Var<char> Char;
typedef Var<int> Int;

/// Quiescence treatment of built-in types to increase interoperability
template<typename T>
void set_symbolic(const T&, const std::string& identifier) {}

/// Quiescence treatment of built-in types to increase interoperability
template<typename T>
void set_symbolic(const T&) {}

/// Force the given variable to be symbolic

/// The name should be unique among the symbolic inputs to the program under
/// test. Changes of the string do not effect the symbolic variable name.
template<typename T>
void set_symbolic(Var<T>& var, const std::string& identifier) {
  var.set_symbolic(identifier);
}

/// Internal non-decreasing counter

/// Count up with each set_symbolic(const T&) call.
extern unsigned int var_seq;

// Internally reusable string builder
static std::stringstream sstream;

/// Set counter for auto generated variable identifiers to zero

/// \see var_seq
/// \see create_identifier()
extern void reset_var_seq();

/// Clear static path constraints

/// Clear the path constraints of the \ref tracer() "global" tracer.
///
/// \see tracer.h
extern void reset_tracer();

/// Create a unique symbolic variable identifier

/// The identifier has the form "Var_N" where N is an non-negative integer
/// which increments with each call.
std::string create_identifier();

/// Force the given variable to be symbolic

/// Symbolic variable identifier is \ref create_identifier() "auto generated".
template<typename T>
void set_symbolic(Var<T>& var) { set_symbolic(var, create_identifier()); }

}

#endif /* VAR_H_ */
