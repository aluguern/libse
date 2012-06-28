// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef VALUE_H_
#define VALUE_H_

#include <memory>
#include <string>
#include <ostream>
#include "expr.h"
#include "tracer.h"

namespace se {

// tracer() returns a static object that can record path constraints.
extern Tracer& tracer();

// TypeTraits is a lookup function that maps primitive types to an enum Type.
// Since template specializations are used, this lookup occurs at compile-time.
template<typename T>
class TypeTraits {};

// Macro to associate a built-in type with an enum Type value.
#define TYPE_TRAITS_DEF(builtin_type, enum_value)\
template<>\
class TypeTraits<builtin_type> {\
  public:\
    static const Type type = enum_value;\
};\

TYPE_TRAITS_DEF(bool, BOOL)
TYPE_TRAITS_DEF(char, CHAR)
TYPE_TRAITS_DEF(int, INT)

// GenericValue is an abstract base class that represents the concrete and (if
// applicable) symbolic value at a specific physical memory address. Concrete
// values are never shared and must be therefore copied by using memcpy or the
// value's copy constructor. Symbolic expressions (if any) are shared until one
// is modified. The type information of a GenericValue object is immutable.
// However, type casts are possible and follow the C++11 language semantics as
// implemented by the C++ compiler with which this library is compiled. In the
// case when type casts were needed and the GenericValue object represents a
// symbolic variable (i.e. is_symbolic() returns true), its current symbolic
// expression is going to be wrapped inside a new CastExpr. A generic value
// that is both symbolic and concrete is also known as a concolic value. If
// a generic value object is concolic, then has_conv_support() returns true.
// A generic value that is symbolic but not concrete, should initially act as
// the identity element in built-in operations.
class GenericValue {

private:

  // type is an immutable field that gives the primitive type information of
  // the represented concrete and (if applicable) symbolic value.
  const Type type;

  // conv_support is a flag that is set to true if and only if any conversion
  // operator in the subclass returns a value that can drive symbolic execution
  // along particular execution paths. Of course, this requires such a value to
  // be consistent with the program semantics of the program being analyzed.
  //
  // Note that this flag must be only updated through the copy constructor or
  // assignment operator. It must never be written by subclasses. However, in
  // the case the flag is true, the subclass must be given clear requirements
  // what updates should occur to its value representation.
  bool conv_support;

  // expr is a shared pointer to a symbolic expression that captures changes to
  // the symbolic value through the overloaded built-in operators. This field
  // is a NULL pointer if and only if the represented value is only concrete.
  SharedExpr expr;

protected:

  // Constructor that creates the representation of a value of the supplied
  // type. If the flag is true, then the subclass must ensure that its
  // value representation through conversion operators is consistent with
  // the program semantics of the program being analyzed.
  GenericValue(const Type type, bool conv_support) :
    type(type), expr(SharedExpr()), conv_support(conv_support) {}

  // Constructor that creates the representation of a value of the supplied
  // type and shared symbolic expression. The latter forces the represented
  // value to be symbolic. If the flag is true, then the subclass must ensure
  // that its value representation through conversion operators is consistent
  // with the program semantics of the program being analyzed.
  GenericValue(const Type type, const SharedExpr expr, bool conv_support) :
    type(type), expr(expr), conv_support(conv_support) {}

  // Copy constructor that creates a deep copy of another value representation.
  // Symbolic expressions are shared since these are expected to be immutable.
  GenericValue(const GenericValue& other) :
    type(other.type), expr(other.expr), conv_support(other.conv_support) {}

  // Assignment operator that assigns the symbolic expression (if any) of the
  // supplied argument to this object. Since GenericValue assignments usually
  // involve one temporary object, no self-assignment check is performed.
  //
  // If has_conv_support() is true, it is the responsibility of the subclass
  // to ensure that subsequent type conversions return a value that is always
  // consistent with the program semantics of the program being analyzed.
  GenericValue& operator=(const GenericValue& other) {
    expr = other.expr;
    return *this;
  }

public:

  // Since GenericValue is a base class, it needs a virtual destructor.
  virtual ~GenericValue() {}

  // get_type() returns the type of the reflected value. The type never changes.
  Type get_type() const { return type; }

  // is_symbolic() returns true if and only if the reflected value are symbolic.
  // A value is symbolic if and only if get_expr() does not return NULL.
  bool is_symbolic() const { return static_cast<bool>(expr); }

  // has_conv_support() returns true if and only if the value or any of the
  // type conversion operators in subclasses return a value that should be
  // able to drive symbolic execution along particular execution paths.
  // This is also known as directed symbolic execution.
  //
  // If has_conv_support() returns true, then the leafs of of the DAG formed
  // by get_expr() are of type ValueExpr<T>. Otherwise, the leafs are of type
  // AnyExpr<T>.
  bool has_conv_support() const { return conv_support; }

  // set_expr(const SharedExpr&) sets the symbolic expression of the value.
  // Unless the argument is NULL, is_symbolic() returns true afterwards.
  void set_expr(const SharedExpr& new_expr) { expr = new_expr; }

  // get_expr() returns the symbolic expression of this value representation.
  // This getter never modifies the expression. The return value is a NULL
  // pointer if and only if is_symbolic() is false.
  virtual const SharedExpr get_expr() const { return expr; }

  // set_symbolic(const std::string&) marks this value as symbolic by either
  // assigning a new AnyExpr<T> or ValueExpr<T>. The choice depends on if this
  // value object supports directed symbolic execution. Recall that this is the
  // case if and only if has_conv_support() returns true.
  virtual void set_symbolic(const std::string&) = 0;

  // write(std::ostream&) writes the concrete value of the value to
  // the supplied output stream.
  virtual std::ostream& write(std::ostream&) const = 0;

};

// __any and __id are internal helper structures used for static compile-time
// overloading of conversion operators. More specifically, it enables finding
// the type of a template parameter. If its type does not matter, use __any.
struct __any {};
template <typename T> struct __id : __any { typedef T type; };

// Value<T> represents a programming value whose type is of primitive type T.
// If GenericValue::has_conv_support() returns true, the value returned by
// Value<T>::get_value() must be able to drive symbolic execution along a
// particular execution path. More accurately, this value must be consistent
// with the program semantics of the program to be analysed. This allows each
// Value<T> object to be used for DART-style symbolic execution. To facilitate
// this, the class implements an implicit T() type conversion operator. This
// conversion is standard except for the bool() case which also must add the
// value's symbolic expression to the path constraints. Currently, the use of
// non-bool types in control flow statements is unsupported (e.g. if(5) {...}).

// Value<T>::get_aux_value() always returns a value that propagates a constant
// expression through an nary expression over an associative and commutative
// binary operator. Note that this auxiliary must be explicitly initialized
// with Value<T>::set_aux_value().
template<typename T>
class Value : public GenericValue {

private:

  // The value field is called the primary value and aux_value is called the
  // auxiliary value. The primary value drives symbolic execution along a
  // particular execution path if and only if GenericValue::has_conv_support()
  // is true. Recall that in this case the primary value must be ensured to
  // be consistent with the program semantics of the program to be analysed.
  // In other words, GenericValue::has_conv_support() returns true if and only
  // if the primary value is defined.
  //
  // The auxiliary value is used to propagate a constant expression within
  // an nary expression over an associative and commutative binary operator.
  // This auxiliary value is defined if and only if has_set_aux() is true.
  //
  // To avoid confusion between the primary and auxiliary value, the value
  // field must only be set through constructors or the assignment operator.
  T value;
  T aux_value;

  // aux_value_init is a flag that indicates if aux_value is defined.
  // It is always initialized to false.
  bool aux_value_init;

  // create_value_expr() returns a shared pointer to a value expression that
  // matches the type and content of the primary value.
  SharedExpr create_value_expr() const {
    return SharedExpr(new ValueExpr<type>(value));
  }

  // create_aux_value_expr() returns a shared pointer to a value expression
  // that matches the type and content of the auxiliary value.
  SharedExpr create_aux_value_expr() const {
    return SharedExpr(new ValueExpr<T>(aux_value));
  }

  // create_value_expr(const std::string&) returns a pointer to a symbolic
  // value expression that matches the type and content of the primary value.
  // The symbolic variable is named according to the supplied string.
  SharedExpr create_value_expr(const std::string& name) const {
    return SharedExpr(new ValueExpr<T>(value, name));
  }

  // create_any_expr(const std::string&) returns a shared pointer to a symbolic
  // expression that references neither the primary nor auxiliary value.
  SharedExpr create_any_expr(const std::string& name) const {
    return SharedExpr(new AnyExpr<T>(name));
  }

  // conv(__any) is a default conversion function that returns the primitive
  // value of type T.
  T conv(__any) const { return value; }

  // conv(__id<bool>) overloads conv(__any). It adds the symbolic expression to
  // the path constraints if and only if is_symbolic() returns true.
  T conv(__id<bool>) const;

public:
  typedef T type;

  // Constructor to create a generic value implementation that supports
  // implicit type conversions of the injected value. Initially, the value
  // has no symbolic expression associated with it. The auxiliary value
  // is undefined until set_aux_value(T) is called.
  Value(const T value) :
      GenericValue(TypeTraits<T>::type, true),
      value(value), aux_value(0), aux_value_init(false) {}

  // Constructor to create a generic value implementation that supports implicit
  // type conversions of the injected value. This value is associated with the
  // symbolic expression given by the second argument. The auxiliary value is
  // undefined until set_aux_value(T) is called.
  Value(const T value, const SharedExpr& expr) :
      GenericValue(TypeTraits<T>::type, expr, true),
      value(value), aux_value(0), aux_value_init(false) {}

  // Constructor to create a generic value implementation that does not support
  // implicit type conversions. Unless a concrete value is assigned to it later,
  // the value is arbitrary and get_value() is undefined. The auxiliary value
  // is undefined until set_aux_value(T) is called.
  Value(const std::string& name) : GenericValue(TypeTraits<T>::type, false),
      value(0), aux_value(0), aux_value_init(false) {
    set_symbolic(name);
  }

  // Copy constructor that creates a value representation based on another.
  Value(const Value& other) : GenericValue(other),
      value(other.value), aux_value(other.aux_value),
      aux_value_init(other.aux_value_init) {}

  // Copy conversion constructor that creates a  value based on another whose
  // type is different from the template parameter T. Thus, a type conversion
  // is required which could result in precision loss. If the supplied value
  // is symbolic, the symbolic expression of the new value is going to be
  // wrapped inside a CastExpr object.
  template<typename S>
  Value(const Value<S>&);

  virtual ~Value() {}

  // has_aux_value() returns true if and only if set_aux_value(T) has been
  // previously called.
  bool has_aux_value() const { return aux_value_init; }

  // get_aux_value() returns the auxiliary value.
  T get_aux_value() const { return aux_value; }

  // set_aux_value(T) initialized the auxiliary value. After this value
  // has been set, get_expr() must return a partial nary expression.
  void set_aux_value(const T v) {
    if(!aux_value_init) { aux_value_init = true; }
    aux_value = v;
  }

  // get_value() returns the primary value. A new value can be assigned
  // through the assignment operator.
  T get_value() const { return value; }

  // Overrides GenericValue::set_symbolic(const std::string&)
  void set_symbolic(const std::string&);

  // Overrides GenericValue::write(std::outstream&)
  std::ostream& write(std::ostream&) const;

  // Unlike GenericValue::get_expr(), Value<T>::get_expr() also seeks to
  // simplify nary expressions over an associative and commutative binary
  // operator. This simplification is implemented through a partial nary
  // expression that are completed using the auxiliary value. For this
  // reason, GenericValue::get_expr() can only return a partial nary
  // expression if has_aux_value() returns true. Note that the converse
  // need not be true.
  const SharedExpr get_expr() const;

  // Assignment operator that copies the concrete value and shared symbolic
  // expression of the supplied value representation. Since the assignment
  // usually involves a temporary value, no self-assignment check is performed.
  Value& operator=(const Value& other) {
    GenericValue::operator=(other);

    value = other.value;
    aux_value = other.aux_value;
    aux_value_init = other.aux_value_init;
    return *this;
  }

  // T() is a const conversion operator. Since C++ supports implicit primitive
  // type conversions, these conversion operators must not be explicit.
  //
  // Note that the bool() conversion operator adds the symbolic expression to
  // the path constraints if and only if is_symbolic() returns true.
  operator T() const { return conv(__id<T>()); }

};

template<typename T>
template<typename S>
Value<T>::Value(const Value<S>& other) :
    GenericValue(TypeTraits<T>::type, other.has_conv_support()),
    value(static_cast<T>(other.get_value())), aux_value(static_cast<T>(other.get_aux_value())) {

  if(other.is_symbolic()) {
    set_expr(SharedExpr(new CastExpr(get_type(), other.get_expr())));
  }
}

template<typename T>
T Value<T>::conv(__id<bool>) const {
  if(is_symbolic()) {
    if(value) {
      tracer().add_path_constraint(get_expr());
    } else {
      tracer().add_path_constraint(SharedExpr(new UnaryExpr(NOT, get_expr())));
    }
  }

  return value;
}

template<typename T>
void Value<T>::set_symbolic(const std::string& name) {
  if(has_conv_support()) {
    set_expr(create_value_expr(name));
  } else {
    set_expr(create_any_expr(name));
  }
}

template<typename T>
std::ostream& Value<T>::write(std::ostream& out) const {
  if(!has_conv_support()) {
    // TODO: Error
  }

  out << value;
  return out;
}

template<typename T>
const SharedExpr Value<T>::get_expr() const {
  if(is_symbolic()) {
    SharedExpr raw_expr = GenericValue::get_expr();
    if(raw_expr->get_kind() == NARY_EXPR) {
      std::shared_ptr<NaryExpr> nary_expr = std::dynamic_pointer_cast<NaryExpr>(raw_expr);
      if(nary_expr->is_partial()) {
        if(nary_expr->is_commutative() && nary_expr->is_associative()) {
          if (!has_aux_value()) {
            // TODO: Error
          }

          NaryExpr* new_nary_expr = new NaryExpr(*nary_expr);
          new_nary_expr->append_expr(create_aux_value_expr());
          return SharedExpr(new_nary_expr);
        }
      }
    }
    return raw_expr;
  }

  if(!has_conv_support()) {
    // TODO: Error
  }

  return create_value_expr();
}

// make_value(T) creates reflection value that represents the concrete value
// given as the argument. Type T should be a primitive type. When the returned
// object is used as a temporary, the compiler is likely to use RVO (return
// value optimization) to avoid the overhead of copying the return value.
template<typename T>
inline Value<T> make_value(const T value) {
  return Value<T>(value);
}

// Macro to declare a function that returns an arbitrary value of the specified
// primitive type. A new object is guaranteed to be instantiated on each call.
// The argument corresponds to the name of the newly created symbolic variable.
#define ANY_DECL(type) \
extern Value<type> any_##type(const std::string& name);

ANY_DECL(bool)
ANY_DECL(char)
ANY_DECL(int)

}

#endif /* VALUE_H_ */
