#ifndef VAR_H_
#define VAR_H_

#include <string>
#include <sstream>
#include "expr.h"
#include "reflect.h"
#include "symtracer.h"

#define SymbolicVarPrefix "Var_"

// Built-in unsigned numerical type which can be initialized to VZERO.
typedef unsigned long long Version;

// Zero according to Version type.
const Version VZERO = 0LL;

// An object of type Var<T> represents a program variable of primitive type T.
// More formally, a Var<T> object corresponds to a C++ lvalue (locator value).
// An lvalue must have an identifiable memory location (i.e. a memory address).
// This memory location points to a concrete value of type T. In addition to
// this, a Var<T> object, whose is_symbolic() member function returns true, can
// refer to an algebraic expression which captures any operations performed on
// the variable. For example, an operation that increments an integer variable
// (i.e. Var<int>) by three yields the symbolic expression "x + 3".
//
// Similar to modifiable lvalues, a Var<T> object can always be changed unless
// it is annotated with the const C++ keyword. Each Var<T> object can also be
// implicitly converted to its underlying concrete value. However, if the
// variable is symbolic such a conversion could indicate that the symbolic
// execution of the program under test is incomplete.
//
// Unlike traditional assignments in imperative programming languages, an
// assignment to a Var<T> object never irrevocably overwrites any information.
// In particular, each variable object has a version number associated with it.
// With each assignment of a new value, the variable's version is incremented.
// This versioning information can be used to create dynamic single assignment
// (DSA) forms.
template<typename /* primitive type */T>
class Var {
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
  Var(const Var<S>& other) : value(other.get_reflect_value()), cast(true),
                             version(VZERO) {}

  // get_reflect_value() returns a read-only reference to an object that
  // contains the concrete value and runtime information about this variable.
  // The caller must ensure that it does not dereference the return value after
  // this Var<T> object has been destroyed.
  const Value<T>& get_reflect_value() const { return value; }

  // get_type() returns the primitive type of this variable. Note that any
  // precision loss that might have occurred due to type casting is
  // conservatively approximated by is_cast().
  Type get_type() const { return ReflectType<T>::type; }

  // is_cast() returns true if either an up or down cast is needed to get the
  // concrete value of the represented variable. In other words, if an unsafe
  // type conversion is needed, then is_cast() returns true. Note that the
  // converse is false since is_cast() also returns true for up casts.
  bool is_cast() const { return cast; }

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
  virtual Var& operator=(const Var& other) {
    if (this != &other) {
      value = other.value;
      cast = other.cast;
      version++;
    }

    return *this;
  }

  // set_symbolic(name) sets this variable as symbolic. The supplied name
  // argument gives the symbolic variable name. This name should be unique.
  // Changes to the string argument after the variable has been set symbolic
  // do not effect the symbolic variable name.
  void set_symbolic(const std::string& name) { value.set_symbolic(name); }

  // is_symbolic() returns true if and only if this variable is symbolic.
  bool is_symbolic() const { return value.is_symbolic(); }

  // get_version() returns a non-negative unsigned number that counts how often
  // the variable has been assigned a new value.
  Version get_version() const { return version; }

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

// reset_tracer() clears the path constraints of the global Tracer object.
// See also symtracer.h
extern void reset_tracer();

// set_symbolic(var) sets the given variable as symbolic with a unique
// symbolic variable identifier. This identifier is of the form Var_%d
// where %d is a non-negative integer that increments with each call.
template<typename T>
void set_symbolic(Var<T>& var) {
  sstream << SymbolicVarPrefix;
  sstream << var_seq;
  var_seq++;

  set_symbolic(var, sstream.str());
  sstream.clear();
}

#endif /* VAR_H_ */
