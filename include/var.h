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

/// Internal base class for a type-safe (symbolic/concrete) lvalue
template<typename T>
class __Var : public AbstractVar {
private:
  bool m_cast;

protected:

  __Var(bool cast) : m_cast(cast) {}
  __Var(const __Var& other) : m_cast(other.m_cast) {}
  virtual ~__Var() {}

  /// Force is_cast() to true

  /// Set to true if and only if value().data() is the result of an up or
  /// downcast. If is_cast() and is_symbolic() is true, then expr() must
  /// have at least one CastExpr.
  void set_cast(bool cast) { m_cast = cast; }

public:

  /// Concrete/symbolic value

  /// Read-only reference to an object that contains the concrete value and
  /// runtime information about this variable. The caller must ensure that
  /// it does not dereference the return value after this object has been
  /// destroyed.
  ///
  /// \see Value
  virtual const Value<T>& value() const = 0;

  Type type() const { return TypeConstructor<T>::type; }
  bool is_cast() const { return m_cast; }
  bool is_concrete() const { return value().is_concrete(); }
  bool is_symbolic() const { return value().is_symbolic(); }
  const SharedExpr expr() const { return value().expr(); }

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
  virtual operator T() const = 0;

  virtual void set_symbolic(const std::string& identifier) = 0;

  virtual Version version() const = 0;
  virtual void set_expr(const SharedExpr& expr) = 0;

  virtual void stash() = 0;
  virtual void unstash(bool restore) = 0;

};

/// Base class for a scalar (symbolic/concrete) lvalue

/// Scalar values are of a fundamental data type such as an integer or pointer.
/// ScalarVar<T> objects cannot be instantiated directly except through a
/// conversion constructor for Value<T> objects. This conversion is needed for
/// the assignment operator, i.e. ScalarVar<T>::operator=(const ScalarVar<T>&).
///
/// Subclasses can safely add new member functions (e.g. array subscript).
/// Subclasses may also override member functions as long as these need not
/// be polymorphic.
template<typename /* primitive type */T>
class ScalarVar : public __Var<T> {
private:
  // stack to stash() and unstash() internal state
  struct State {
    const Value<T> value;
    const bool cast;
    const Version version;
  };
  std::stack<State> m_stack;

protected:
  // Concrete and/or symbolic value of this variable
  Value<T> m_value;

  // Version number that never decreases; initially, it is VZERO. With each
  // assignment operation, the version number is incremented by one.
  Version m_version;

  /// Concrete variable
  ScalarVar(const T data) : __Var<T>(false), m_value(data), m_version(VZERO) {}

  /// Variable based on another (symbolic/concrete) value of the same type
  ScalarVar(const Value<T>& value, Version version) : __Var<T>(false),
    m_value(value), m_version(version) {}

  /// Variable based on another (symbolic/concrete) value of a different type

  /// Since the type of the value differ from the template type T,
  /// the new variable's concrete data could be subject to precision loss;
  /// this is conservatively approximated by is_cast(). Moreover, if the
  /// value is_symbolic(), the symbolic expression of the variable is going
  /// to have a new CastExpr.
  template<typename S>
  ScalarVar(const Value<S>& value) : __Var<T>(true), m_value(value),
    m_version(VZERO) {}

  /// Safe copy constructor

  /// Copy constructor that instantiates a variable by creating a deep copy of
  /// the supplied variable's concrete data. Any symbolic expressions are
  /// shared between both the original and copied variable object until either
  /// one is modified. Note that casts are transitive: if other.is_cast() is
  /// true, then the copy's is_cast() is true.
  ScalarVar(const ScalarVar& other) : __Var<T>(other), m_value(other.m_value),
    m_version(VZERO) {}

  /// Unsafe copy constructor with type casting

  /// Copy conversion constructor that creates a copy of another variable.
  /// Incompatibilities between the type of the supplied variable's concrete
  /// data and the type of the instantiated variable object are resolved by
  /// implicit type casting. In the case in which the other variable is also
  /// symbolic, the new variable's expr() is going to have a new CastExpr.
  template<typename S>
  ScalarVar(const ScalarVar<S>& other) : __Var<T>(true), m_value(other.value()),
    m_version(VZERO) {}

public:

  /// Variable based on another (symbolic/concrete) value of the same type
  ScalarVar(const Value<T>& value) : __Var<T>(false), m_value(value),
    m_version(VZERO) {}

  virtual ~ScalarVar() {}

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
    if(__Var<T>::is_symbolic()) {
       // TODO: log possibility of incomplete analysis
    }

    return m_value.data();
  }

  /// Replace the value and propagate cast information

  /// Copy concrete data and symbolic expression (if any) of the other variable.
  /// Note that when the variable is assigned to itself, no data is copied.
  /// Thus, self-assignments also leave the version number unchanged.
  ///
  /// The class ensures that any required type coercion of the argument is first
  /// done by its converting constructor, i.e. ScalarVar<T>(const ScalarVar<U>&).
  ScalarVar& operator=(const ScalarVar& other) {
    if (this != &other) {
      set_cast(other.is_cast());
      m_value = other.value();
      m_version++;
    }

    return *this;
  }

  Version version() const { return m_version; }

  void set_symbolic(const std::string& identifier) {
    m_value.set_symbolic(identifier);
  }

  void set_expr(const SharedExpr& expr) {
    m_version++;
    m_value.set_expr(expr);
  }

  void stash() { m_stack.push(State{ m_value, __Var<T>::is_cast(), m_version }); }
  void unstash(bool restore) {
    if (restore) {
      const State& state = m_stack.top();
      if (m_version != state.version) {
        set_cast(state.cast);
        m_value = state.value;
        m_version++;
      }
    }

    m_stack.pop();
  }
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
///   Code                                                                           | Description 
/// -------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------
///  Var<T> x = \ref any(const std::string&) "any"<T>("X")                           | \ref is_symbolic() "symbolic" variable of type `T` and identifier "X"
///  Var<T> x = [value]                                                              | \ref is_concrete() "concrete" variable of type `T`
///  Var<T> x = [value]; x.\ref set_symbolic(const std::string&) "set_symbolic"("X") | variable of type `T` which is \ref is_symbolic() "symbolic" and \ref is_concrete() "concrete"
///
/// Once a variable has been created, it can be used as any other regular
/// program variable as long as the required operators have been overloaded.
template<typename T>
class Var : public ScalarVar<T> {
public:

  Var(const T data) : ScalarVar<T>(data) {}
  Var(const Value<T>& value) : ScalarVar<T>(value) {}

  template<typename S>
  Var(const Value<S>& value) : ScalarVar<T>(value) {};

  Var(const Var& other) : ScalarVar<T>(other) {}

  template<typename S>
  Var(const Var<S>& other) : ScalarVar<T>(other) {}

  ~Var() {}

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
