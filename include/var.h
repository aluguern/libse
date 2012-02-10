#ifndef VAR_H_
#define VAR_H_

#include <string>
#include <sstream>
#include "expr.h"
#include "reflect.h"
#include "symtracer.h"

#define SymbolicVarPrefix "Var_"

// An object of type Var<T> represents a program variable of primitive type T.
// More formally, a Var<T> object corresponds to a C++ lvalue (locator value).
// An lvalue must have an identifiable memory location (i.e. a memory address).
// This memory location points to a concrete value of type T. In addition to
// this, a Var<T> object can also refer to an algebraic expression which is the
// result of performing operations on its concrete value. In that case, we say
// that the variable is symbolic as indicated by the is_symbolic() function.
//
// Similar to modifiable lvalues, a Var<T> object can always be changed unless
// it is annotated with the const C++ keyword. Each Var<T> object can also be
// implicitly converted to its underlying concrete value. However, if the
// variable is symbolic such a conversion could indicate that the symbolic
// execution of the program under test is incomplete.
template<typename /* primitive type */T>
class Var {
private:
  // value represents the concrete value at a physical memory address. In
  // addition, there is a symbolic expression for this concrete value if
  // and only if this variable is symbolic (i.e. is_symbolic() returns true).
  Value<T> value;

  // cast flag is true if and only if the represented concrete value was
  // obtained through an up or downcast. If the variable is symbolic, it is
  // also going to have a CastExpr which indicates the required type cast.
  bool cast;

public:

  // Constructor creates an object that represents a program variable of a
  // primitive type T with the given initial value. The instantiated variable
  // is concrete (i.e. not symbolic).
  Var(const T concrete_value) : value(concrete_value), cast(false) {}

  // Internal constructor that creates an object that represents a program
  // variable that has the same concrete value and (if any) symbolic value as
  // the variable pointed to by the supplied argument. Since the type of the
  // pointed to value representation could differ from the template type T,
  // the instantiated variable object could be subject to precision loss which
  // is conservatively approximated by is_cast(). Moreover, if the variable is
  // symbolic, the instantiated variable is going to have a new CastExpr.
  Var(const ReflectValue::Pointer& value_ptr);

  // Copy constructor that instantiates a variable by creating a deep copy of
  // the supplied variable's concrete value. Any symbolic expressions are
  // shared between both the original and copied variable object until either
  // one is modified. Note that casts are transitive: if other.is_cast() is
  // true, then the copy's is_cast() is true.
  Var(const Var& other) : value(other.value), cast(other.cast) {}

  // Copy conversion constructor that creates a copy of another variable.
  // Incompatibilities between the type of the supplied variable's concrete
  // value and the type of the instantiated variable object are resolved by
  // implicit type casting. In the case in which the other variable is also
  // symbolic, the instantiated variable is going to have a new CastExpr.
  template<typename S>
  Var(const Var<S>& other);

  // get_reflect_value() returns a read-only reference to an object that
  // contains runtime information about this variable. The caller must
  // ensure that it does not dereference the return value after this
  // Var<T> object has been destroyed.
  const Value<T>& get_reflect_value() const { return value; }

  // get_type() returns the primitive type of this variable. Any precision loss
  // that might have resulted from type casting is conservatively approximated
  // by is_cast().
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

    return static_cast<T>(value);
  }

  // Assignment operator that copies both the concrete and symbolic bytes of
  // the supplied variable. Any required type conversions are first performed
  // by the copy conversion constructor. These conversions could result in
  // precision lost. In that case, is_cast() returns true after the assignment
  // operator has completed the copying of the underlying bytes.
  virtual Var& operator=(const Var& other) {
    if (this != &other) {
      value = other.value;
      cast = other.cast;
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

};

template<typename T>
Var<T>::Var(const ReflectValue::Pointer& value_ptr) :
  value(), cast(ReflectType<T>::type != value_ptr->get_type()) {

  if(value_ptr->is_symbolic()) {
    if(cast) {
      const Type type = ReflectType<T>::type;
      const SharedExpr& expr = value_ptr->get_expr();
      const SharedExpr& cast_expr = SharedExpr(new CastExpr(expr, type));
      value.set_expr(cast_expr);
    } else {
      value.set_expr(value_ptr->get_expr());
    }
  }

  // implicit casting of the other concrete value
  value_ptr->copy_value(value);
}

template<typename T>
template<typename S>
Var<T>::Var(const Var<S>& other) : value(), cast(true) {
  if(other.is_symbolic()) {
    const Type type = ReflectType<T>::type;
    const SharedExpr& other_expr = other.get_reflect_value().get_expr();
    const SharedExpr& cast_expr = SharedExpr(new CastExpr(other_expr, type));

    value.set_expr(cast_expr);
  }

  // implicit casting of the other concrete value
  value.set_value(other.get_reflect_value().get_value());
}

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

// Internal function to guide the type inference of template functions that
// overload built-in operators such as addition, multiplication etc.
template<typename T>
static inline const ReflectValue& __filter(const Var<T>& var) {
  return var.get_reflect_value();
}

// Internal function to guide the type inference of template functions that
// overload built-in operators such as addition, multiplication etc.
// For comments about the pass-by-reference of the shared pointer, refer to
// the OVERLOAD_OPERATION documentation.
static inline const ReflectValue& __filter(const ReflectValue::Pointer& pointer) {
  return *pointer;
}

// OVERLOAD_OPERATION(op) is a macro that uses templates and the statically
// overloaded and inlined __filter function to implement a custom C++ operator.
// Note that the template types can be shared pointers. However, since the
// operator operands are passed by reference (to avoid copying Var<T> objects),
// their reference counts are not going to be incremented. This is safe and no
// dangling references can occur because none of these overload implementations
// have any side-effects on their operands. To see this, inspect the Value<T>
// operator overloads which are guaranteed to never reset any pointers.
#define OVERLOAD_OPERATION(op) \
  template<typename X, typename Y>\
  const ReflectValue::Pointer operator op(const X& x, const Y& y) {\
    return __filter(x) op __filter(y);\
  }\
  template<typename X>\
  const ReflectValue::Pointer operator op(const X& x, const int y) {\
    /* force double dispatch to preserve operand ordering */\
    const ReflectValue& __y = Value<int>(y);\
    return __filter(x) op __y;\
  }\
  template<typename Y>\
  const ReflectValue::Pointer operator op(const int x, const Y& y) {\
    return Value<int>(x) op __filter(y);\
  }\

OVERLOAD_OPERATION(+)
OVERLOAD_OPERATION(<)

#endif /* VAR_H_ */
