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

static const std::string SymbolicVarPrefix = "Var_";

// Built-in unsigned numerical type which can be initialized to VZERO.
typedef unsigned long long Version;

// Zero according to Version type.
const Version VZERO = 0LL;

// GenericVar is an interface which represents a symbolic program variable.
// More formally, a GenericVar associates a C++ lvalue (locator value) with
// an algebraic expression in a theory. For example, a program statement that
// increments a variable by three could yield the symbolic expression "x + 3"
// in the ring of integers. By choosing an appropriate theory, the GenericVar
// API can be used to successively approximate the strongest postcondition of
// a program. This is typically referred to as symbolic execution.
//
// Thus, if a GenericVar object has a symbolic expression, an assignment to it
// never irrevocably overwrites any information. This contrasts traditional
// assignments in imperative programming languages. For this reason, the API
// can be also useful for debugging.
//
// Changes to the variable are tracked by a version number. The version number
// is always initialized to zero. Subsequently, on each modifier operation, it
// is incremented by one. These version numbers are never modified based on the
// version information of another GenericVar object. This makes the version
// information applicable for dynamic single assignment (DSA) forms.
//
// Except from this versioning information, the internal state of a variable
// can be saved and restored with stash() and unstash(true), respectively.
// It is a programmer error to call unstash(bool) more times than stash().
// To conserve resources, every stash() call should be matched with a
// corresponding unstash(bool) call (note that the flag value is irrelevant
// for resource deallocation purposes)
//
// Finally, each implementation of the GenericVar interface must take type
// casting into account. These type casts should be explicit in symbolic
// expressions. The proper way of doing this is through the CastExpr API.
class GenericVar {

public:

  // get_type() returns the primitive type of this variable. Note that any
  // precision loss that might have occurred due to type casting is
  // conservatively approximated by is_cast().
  virtual Type get_type() const = 0;

  // is_cast() returns true if either an up or down cast is needed to get the
  // concrete value of the represented variable. In other words, if an unsafe
  // type conversion is needed, then is_cast() returns true. Note that the
  // converse is false since is_cast() also returns true for up casts.
  virtual bool is_cast() const = 0;

  // set_symbolic(name) sets this variable as symbolic. The supplied name
  // argument gives the symbolic variable name. This name should be unique.
  // Changes to the string argument after the variable has been set symbolic
  // do not effect the symbolic variable name.
  virtual void set_symbolic(const std::string& name) = 0;

  // is_symbolic() returns true if and only if this variable is symbolic.
  virtual bool is_symbolic() const = 0;

  // get_version() returns a non-negative unsigned number that counts how often
  // the variable has been assigned a new value or symbolic expression.
  virtual Version get_version() const = 0;

  // set_expr(new_expr) sets the symbolic expression of the variable.
  // Unless new_expr is a NULL pointer, is_symbolic() returns true afterwards.
  // This setter increments the version number by one each time it is called.
  virtual void set_expr(const SharedExpr& new_expr) = 0;

  // get_expr() returns the symbolic expression of the variable.
  // The return value is a NULL pointer if and only if is_symbolic() is false.
  virtual const SharedExpr get_expr() const = 0;

  // stash() saves the internal state of this variable. All version information
  // remains unaffected by this call.
  virtual void stash() = 0;

  // unstash(true) restores the most recently stashed internal variable state.
  // It also deallocates all the resources which stored this internal state.
  // The variable's version number is incremented by one if and only if it has
  // been modified since the most recent stash() call.
  //
  // In contrast, unstash(false) only deallocates any resources acquired by the
  // most recent stash() call - the internal variable state remains unaffected.
  // Thus, the versioning information never changes when the flag is false.
  //
  // Regardless of the flag value, it is a programmer error to call
  // unstash(bool) more times than stash() has been invoked.
  virtual void unstash(bool) = 0;

  // Every interface needs a public virtual destructor.
  virtual ~GenericVar() {}
};

// A Var<T> object is a GenericVar object which can have also a concrete value
// of primitive type T associated with it. However, this value is undefined if
// the variable has been initialized to an object of type AnyValue<T>. This is
// done when the variable should be purely symbolic.
//
// Another possibility is that the Var<T> object has both a concrete value and
// a symbolic expression. This is useful for symbolic execution augmented with
// runtime information (also known as concolic execution). This feature can be
// enabled by initializing the Var<T> object to a concrete value and calling
// the set_symbolic(const std::string&) member function on it.
//
// However, note that implicit conversions of a Var<T> object to its underlying
// concrete value (if defined) could be indication of an incomplete analysis.
template<typename /* primitive type */T>
class Var : public GenericVar {
 
private:

  // value represents the concrete value at a physical memory address. In
  // addition, there is a symbolic expression for this concrete value if
  // and only if this variable is symbolic (i.e. is_symbolic() returns true).
  Value<T> value;

  // cast flag is true if and only if the represented concrete value was
  // obtained through an up or downcast. If the variable is symbolic and cast
  // is true, it is also going to have a CastExpr for the required type cast.
  bool cast;

  // Version number that never decreases; initially, it is VZERO. With each
  // assignment operation, the version number is incremented by one.
  Version version;

  // stack to stash() and unstash() the internal state of this Var<T> object.
  struct State { const Value<T> value; const bool cast; Version version; };
  std::stack<State> stack;

public:

  // Constructor creates an object that represents a program variable of a
  // primitive type T with the given initial value. Initially, the new
  // variable is concrete (i.e. not symbolic).
  Var(const T concrete_value) : value(concrete_value), cast(false),
                                version(VZERO) {}

  // Internal constructor that creates an object that represents a program
  // variable that has the same concrete value and (if any) symbolic value as
  // the variable pointed to by the supplied argument. Since both reflection
  // types match, no type casting is performed.
  Var(const Value<T>& value) : value(value), cast(false),
                               version(VZERO) {}

  // Internal constructor that creates an object that represents a program
  // variable that has the same concrete value and (if any) symbolic value as
  // the variable pointed to by the supplied argument. Since the type of the
  // pointed to value representation differ from the template type T, the
  // instantiated variable object could be subject to precision loss which
  // is conservatively approximated by is_cast(). Moreover, if the variable
  // is symbolic, the instantiated variable is going to have a new CastExpr.
  template<typename S>
  Var(const Value<S>& value) : value(value), cast(true), version(VZERO) {}

  // Copy constructor that instantiates a variable by creating a deep copy of
  // the supplied variable's concrete value. Any symbolic expressions are
  // shared between both the original and copied variable object until either
  // one is modified. Note that casts are transitive: if other.is_cast() is
  // true, then the copy's is_cast() is true.
  Var(const Var& other) : value(other.value), cast(other.cast),
                          version(VZERO) {}

  // Copy conversion constructor that creates a copy of another variable.
  // Incompatibilities between the type of the supplied variable's concrete
  // value and the type of the instantiated variable object are resolved by
  // implicit type casting. In the case in which the other variable is also
  // symbolic, the instantiated variable is going to have a new CastExpr.
  template<typename S>
  Var(const Var<S>& other) : value(other.get_value()), cast(true),
                             version(VZERO) {}

  ~Var() {}

  // get_value() returns a read-only reference to an object that
  // contains the concrete value and runtime information about this variable.
  // The caller must ensure that it does not dereference the return value after
  // this Var<T> object has been destroyed.
  const Value<T>& get_value() const { return value; }

  // T() is a conversion operator that returns the concrete value of this
  // variable. This conversion abides to the C++ type casting rules which are
  // unsafe. If such an unsafe type casting is needed, is_cast() returns true.
  // Also note that the conversion of a symbolic variable to a concrete value
  // could render symbolic execution incomplete.
  operator T() const {
    if(is_symbolic()) {
       // TODO: log possibility of incomplete analysis
    }

    return value.get_value();
  }

  // Assignment operator that copies both the concrete and symbolic bytes of
  // the supplied variable. Any required type conversions are first performed
  // by the copy conversion constructor. These conversions could result in
  // precision lost. In that case, is_cast() returns true after the assignment
  // operator has completed the copying of the underlying bytes. When the
  // variable is assigned to itself, no data is copied. In that case, the
  // variable's version number is not incremented.
  Var& operator=(const Var& other) {
    if (this != &other) {
      value = other.value;
      cast = other.cast;
      version++;
    }

    return *this;
  }

  Type get_type() const { return TypeTraits<T>::type; }
  bool is_cast() const { return cast; }
  void set_symbolic(const std::string& name) { value.set_symbolic(name); }
  bool is_symbolic() const { return value.is_symbolic(); }
  Version get_version() const { return version; }
  const SharedExpr get_expr() const { return value.get_expr(); }
  void set_expr(const SharedExpr& expr) { version++; value.set_expr(expr); }

  void stash() { stack.push(State{ value, cast, version }); }
  void unstash(bool restore) {
    if (restore) {
      const State& state = stack.top();
      if (version != state.version) {
        cast = state.cast;
        value = state.value;
        version++;
      }
    }

    stack.pop();
  }
};

typedef Var<bool> Bool;
typedef Var<char> Char;
typedef Var<int> Int;

// Quiescence treatment of built-in types to increase interoperability
template<typename T>
void set_symbolic(const T&, const std::string& name) {}

// Quiescence treatment of built-in types to increase interoperability
template<typename T>
void set_symbolic(const T&) {}

// set_symbolic(var, name) sets the given variable as symbolic and assigns a
// name to it that is used as part of symbolic expressions. This name should
// be unique among the symbolic variables in the program under test.
// Changes of the supplied string do not effect the symbolic variable name.
template<typename T>
void set_symbolic(Var<T>& var, const std::string& name) {
  var.set_symbolic(name);
}

// var_seq is an internal incrementing counter to construct unique symbolic
// variable names as part of set_symbolic(var) invocations.
extern unsigned int var_seq;

// sstream is an internally reusable string builder.
static std::stringstream sstream;

// reset_var_seq() sets the counter which is used by create_var_name() to zero.
extern void reset_var_seq();

// reset_tracer() clears the path constraints of the global Tracer object.
// See also tracer.h
extern void reset_tracer();

// create_var_name() returns a unique symbolic variable identifier which
// has the form Var_%d where %d is an increasing non-negative integer.
std::string create_var_name();

// set_symbolic(var) sets the given variable as symbolic with a unique
// symbolic variable identifier. This identifier is of the form Var_%d
// where %d is a non-negative integer that increments with each call.
template<typename T>
void set_symbolic(Var<T>& var) { set_symbolic(var, create_var_name()); }

}

#endif /* VAR_H_ */
