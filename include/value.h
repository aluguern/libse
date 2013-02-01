// Copyright 2012-2013, Alex Horn. All rights reserved.
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

/*! \file value.h */

/// Static object which can record path constraints
extern Tracer& tracer();

/// Static type lookup table

/// Example:
///
///     TypeConstructor<char>::type == CHAR
///
/// \see Type
template<typename T>
class TypeConstructor {};

/// Define a type constructor for a built-in primitive type
#define TYPE_CTOR_DEF(builtin_type, enum_value)\
template<>\
class TypeConstructor<builtin_type> {\
  public:\
    static const Type type = enum_value;\
};\

TYPE_CTOR_DEF(bool, BOOL)
TYPE_CTOR_DEF(char, CHAR)
TYPE_CTOR_DEF(int, INT)

/// Base class for a (symbolic/concrete) rvalue

/// Base class of an rvalue with optional concrete data and an optional
/// symbolic expression.
///
/// A value is called symbolic if and only if is_symbolic() is true. A
/// symbolic value's \ref AbstractValue::expr() "expression" is always
/// \ref SharedExpr "shared". For this reason, expressions themselves
/// should never be modified.
///
/// The primary motivation for a symbolic value is the ability to implement
/// multi-path (i.e. CBMC-style) symbolic execution of a program under test.
/// This requires source annotations of the program with Loop and If objects.
/// This should be automated through a source-to-source transformation.
///
/// A value is said to be concolic if and only if it is \ref is_symbolic()
/// "symbolic" and \ref is_concrete() "concrete". That is, a concolic value
/// has both a symbolic expression and concrete data. This is the basis for
/// single-path (i.e. DART-style) symbolic execution. A value's concrete data
/// must never be modified; it should only be copied by using memcpy or the
/// concrete value's copy constructor.
///
/// Even though a value's type() is immutable, type casts are possible and
/// follow the C++11 language semantics as implemented by the C++ compiler
/// with which this library is compiled. Any type cast which coerces a
/// symbolic value is explicit in its symbolic expression through a CastExpr.
///
/// \remark For type safety reasons, the concrete data (if any) is not accessible
/// through this abstract class. Instead, refer to a documentation of a
/// specific subclass on how to access the concrete data.
class AbstractValue {

private:

  // Immutable type information about the concrete and/or symbolic value.
  const Type m_type;

  // Flag is true if and only if any conversion operator in a subclass returns
  // concrete data that is consistent with the semantics of the operations.
  //
  // Note that this flag must be only updated through the copy constructor or
  // assignment operator; it must never be written by subclasses. If the flag
  // is true, subclasses are expected to adhere to the contract given in the
  // protected constructors and is_concrete() member function.
  bool m_concolic;

  // Shared pointer to a symbolic expression that captures changes to the
  // symbolic value through the overloaded built-in operators. This field
  // is a NULL pointer if and only the value is not symbolic.
  SharedExpr m_expr;

protected:

  /// Concrete value with type information

  /// Set `concolic` to true when the subclass can ensure that the concrete
  /// value is consistent with the program semantics of the program under test.
  AbstractValue(const Type type, bool concolic) :
    m_type(type), m_expr(SharedExpr()), m_concolic(concolic) {}

  /// Concrete value with type information and symbolic expression

  /// The symbolic expression forces the represented value to be symbolic. Set
  /// `concolic` to true when the subclass can ensure that the concrete data
  /// is consistent with the program semantics of the program under test.
  AbstractValue(const Type type, const SharedExpr expr, bool concolic) :
    m_type(type), m_expr(expr), m_concolic(concolic) {}

  /// Copy type information and symbolic expression (if any) 

  /// \remark Subclasses must copy any concrete data 
  AbstractValue(const AbstractValue& other) :
    m_type(other.m_type), m_expr(other.m_expr), m_concolic(other.m_concolic) {}

  /// Replace symbolic expression

  /// Since assignments usually involve a temporary object, no self-assignment
  /// check is performed.
  ///
  /// If the other value is_concrete(), it is the responsibility of the
  /// subclass to ensure that subsequent type conversions return a value that
  /// is consistent with the program semantics of the program under test.
  AbstractValue& operator=(const AbstractValue& other) {
    m_expr = other.m_expr;
    return *this;
  }

public:

  // Since AbstractValue is a base class, it needs a virtual destructor.
  virtual ~AbstractValue() {}

  /// type information

  /// \remark type information never changes
  Type type() const { return m_type; }

  /// Is symbolic expression defined?

  /// true if and only if expr() does not return NULL. If is_symbolic() is
  /// false, then is_concrete() is true. In general, the converse is false.
  bool is_symbolic() const { return static_cast<bool>(m_expr); }

  /// Is concrete data defined?

  /// true if and only if the concrete data is consistent with the program
  /// semantics of the program under test. If is_concrete() is false, then
  /// the value is only suitable for multi-path symbolic execution.
  bool is_concrete() const { return m_concolic; }

  /// Set the symbolic expression of the value

  /// Unless the argument is NULL, is_symbolic() returns true afterwards.
  void set_expr(const SharedExpr& expr) { m_expr = expr; }

  /// Unsimplified symbolic expression

  /// Expression as it has been \ref set_expr(const SharedExpr&) "set".
  /// The return value is a NULL pointer if and only if is_symbolic() is false.
  ///
  /// \remark Subclasses must never modify the expression once it has been
  ///         returned
  virtual const SharedExpr expr() const { return m_expr; }

  /// Force value to be symbolic

  /// Create symbolic value by either creating a new AnyExpr or ValueExpr.
  /// The choice depends on is_concrete().
  virtual void set_symbolic(const std::string&) = 0;

  /// Serialize concrete data and/or symbolic expression
  virtual std::ostream& write(std::ostream&) const = 0;

};

/// Internal base class for a type-safe (symbolic/concrete) rvalue
///
/// This interface guarantees that if AbstractValue::is_concrete(), then any
/// concrete data returned by __Value<T>::data() must be consistent with the
/// program semantics of the program under test. This allows such a value to
/// be used for single-path (i.e. DART-style) symbolic execution. Recall that
/// this is also known as concolic execution. To facilitate this, a subclass
/// must implements an implicit T() type conversion operator. This conversion
/// should be standard except for the bool() case: it must also add the value's
/// symbolic expression to the list of \ref tracer() "path constraints". Thus,
/// the use of non-bool types in control-flow such as `if (i + 5) {...}` is
/// currently unsupported in this implementation of concolic execution.
///
/// If has_aggregate() is true, then aggregate() returns concrete data which
/// is the result of constant propagation within the value's nary expression
/// built over an associative and commutative binary operator.
template<typename T>
class __Value : public AbstractValue {
protected:

  /// Concrete value for multi-path and single-path symbolic execution

  /// Initially, there is no symbolic expression associated with the value.
  /// The aggregate is undefined until set_aggregate(const T) is called.
  __Value(bool concolic) : AbstractValue(TypeConstructor<T>::type, concolic) {}

  /// Concolic value for multi-path and single-path symbolic execution

  /// The aggregate is undefined until set_aggregate(const T) is called.
  __Value(bool concolic, const SharedExpr& expr) :
      AbstractValue(TypeConstructor<T>::type, expr, concolic) {}

  /// Copy any concrete data and symbolic expression
  __Value(const __Value& other) : AbstractValue(other) {}

  /// Copy conversion constructor with type casting

  /// Copy data() and aggregate() values of another type. Since this requires a
  /// type conversion, precision could be lost. However, if the other value is
  /// symbolic, the type cast is explicit in form of a CastExpr.
  template<typename S>
  __Value(const __Value<S>&);

public:

  /// Is symbolic expression simplified through constant propagation?

  /// true if and only if set_aggregate(const T) has been called at least once.
  ///
  /// \remark if expr() returns a
  ///         \ref NaryExpr::is_partial() "partial nary expression",
  ///         then has_aggregate() is true.
  ///
  /// \see aggregate()
  /// \see set_aggregate(const T)
  // TODO: Support pointer modifications, i.e. (int* i) + N
  virtual bool has_aggregate() const = 0;

  /// Current result of constant propagation (if defined)

  /// This auxiliary value is defined if and only if has_aggregate() is
  /// true. This aggregate data is used to propagate constants. This
  /// form of constant propagation simplifies the value's
  /// \ref expr() "symbolic expression" if it is an NaryExpr over an
  /// \ref NaryExpr::is_associative() "associative" and
  /// \ref NaryExpr::is_commutative() "commutative" binary operator.
  ///
  /// Thus, if has_aggregate() is true, then is_symbolic() is true.
  /// But is_concrete() can be false. In that case, the concrete data is
  /// interpreted as the identity element of the nary operator.
  ///
  /// \see has_aggregate()
  /// \see set_aggregate(const T)
  virtual T aggregate() const = 0;

  /// Set constant propagation value

  /// Since aggregate() is meant to simplify the value's \ref expr()
  /// "symbolic expression", is_symbolic() must be true when calling
  /// set_aggregate(const T).
  ///
  /// \remark has_aggregate() returns true afterwards
  /// \see aggregate()
  virtual void set_aggregate(const T data) = 0;

  /// Concrete data (if defined)

  /// Concrete data is defined if and only if is_concrete() return true.
  /// \remark Concrete data can only be set through the assignment operator
  virtual T data() const = 0;

  /// Concrete data (if defined)

  /// The return value is defined if and only if is_concrete() is true.
  /// Note that if is_symbolic() is true, the conversion could render
  /// concolic execution incomplete.
  ///
  /// Since C++ supports implicit primitive type conversions, this conversion
  /// operator is not explicit. Note that the bool() conversion operator adds
  /// the symbolic expression to the path constraints if and only if
  /// is_symbolic() returns true.
  ///
  /// \see is_concrete()
  virtual operator T() const = 0;

  virtual ~__Value() {};
};

// __any and __id are internal helper structures used for static compile-time
// overloading of conversion operators. More specifically, it enables finding
// the type of a template parameter. If its type does not matter, use __any.
struct __any {};
template <typename T> struct __id : __any { typedef T type; };

/// Base class for a scalar (symbolic/concrete) rvalue

/// Scalar values are of a fundamental data type such as an integer or pointer.
/// ScalarValue<T> objects cannot be instantiated directly.
///
/// Subclasses should not override any member functions. However, subclasses
/// may add new member functions (e.g. array subscript).
///
/// \see any(const std::string&)
/// \see make_value(const T)
template<typename T>
class ScalarValue : public __Value<T> {
private:

  // The m_data field is used for concolic execution. That is, if
  // AbstractValue::is_concrete() is true, then the m_data must be
  // consistent with the program semantics of the program under test.
  // In other words, if defined, m_data corresponds to a value
  // computed along a single execution path in the program under test.
  //
  // The field m_aggregate is used to propagate constants. It is
  // defined if and only if has_aggregate() is true. If has_aggregate()
  // is true, then is_symbolic() is also true.
  //
  // To avoid confusion between these two fields, m_data can only be
  // set through constructors or the assignment operator.
  T m_data;
  T m_aggregate;

  // Is m_aggregate defined? Flag is always initialized to false.
  bool m_aggregate_init;

  // Create shared pointer to a new symbolic expression that matches the type
  // and content of data(). A pristine symbolic variable identifier is
  // automatically generated.
  SharedExpr create_value_expr() const {
    return SharedExpr(new ValueExpr<T>(m_data));
  }

  // Create shared pointer to a new symbolic expression that matches the return
  // type and content of aggregate().
  SharedExpr create_aggregate_expr() const {
    return SharedExpr(new ValueExpr<T>(m_aggregate));
  }

  // Create shared pointer to a new symbolic expression that matches the return
  // type and value of data(). Unlike create_value_expr(), the symbolic variable
  // is identified by the supplied string.
  SharedExpr create_value_expr(const std::string& identifier) const {
    return SharedExpr(new ValueExpr<T>(m_data, identifier));
  }

  // Create shared pointer to a new symbolic expression that relies
  // neither on data() nor aggregate().
  SharedExpr create_any_expr(const std::string& identifier) const {
    return SharedExpr(new AnyExpr<T>(identifier));
  }

  // Default conversion function that returns the underlying concrete data
  T conv(__any) const { return m_data; }

  // Overloads conv(__any). It adds the symbolic expression to the path
  // constraints if and only if is_symbolic() is true.
  T conv(__id<bool>) const;

protected:

  /// Concrete value for multi-path and single-path symbolic execution

  /// Initially, there is no symbolic expression associated with the value.
  /// The aggregate is undefined until set_aggregate(const T) is called.
  ScalarValue(const T data) : __Value<T>(true), m_data(data), m_aggregate(0),
    m_aggregate_init(false) {}

  /// Concolic value for multi-path and single-path symbolic execution

  /// The aggregate is undefined until set_aggregate(const T) is called.
  ScalarValue(const T data, const SharedExpr& expr) : __Value<T>(true, expr),
    m_data(data), m_aggregate(0), m_aggregate_init(false) {}

  /// Arbitrary value only for multi-path symbolic execution

  /// Since the value does not support implicit type conversions, it is only
  /// suitable for multi-path symbolic execution. Unless a concrete value is
  /// assigned to it later, data() is undefined. Also aggregate() is
  /// undefined until set_aggregate(const T) is called.
  ScalarValue(const std::string& identifier) : __Value<T>(false), m_data(0),
    m_aggregate(0), m_aggregate_init(false) { set_symbolic(identifier); }

  /// Copy any concrete data and symbolic expression
  ScalarValue(const ScalarValue& other) : __Value<T>(other),
      m_data(other.m_data), m_aggregate(other.m_aggregate),
      m_aggregate_init(other.m_aggregate_init) {}

  /// Copy conversion constructor with type casting

  /// Copy data() and aggregate() values of another type. Since this requires a
  /// type conversion, precision could be lost. However, if the other value is
  /// symbolic, the type cast is explicit in form of a CastExpr.
  template<typename S>
  ScalarValue(const ScalarValue<S>&);

public:

  virtual ~ScalarValue() {}

  bool has_aggregate() const { return m_aggregate_init; }
  T aggregate() const { return m_aggregate; }

  void set_aggregate(const T data) {
    if(!m_aggregate_init) { m_aggregate_init = true; }
    m_aggregate = data;
  }

  T data() const { return m_data; }

  // Overrides AbstractValue::set_symbolic(const std::string&)
  void set_symbolic(const std::string&);

  // Overrides AbstractValue::write(std::outstream&)
  std::ostream& write(std::ostream&) const;

  /// Simplified symbolic expression (if defined)

  /// Unlike AbstractValue::expr(), ScalarValue<T>::expr() seeks to simplify
  /// an nary expression over an associative and commutative binary operator.
  /// This simplification is implemented through a \ref NaryExpr::is_partial()
  /// "partial nary expression" which is completed using the aggregate() value.
  /// For example, the symbolic expression "x + 2 + 3" would set aggregate()
  /// to five (5 = 2 + 3). Then, expr() would return a \ref SharedExpr to a
  /// new immutable NaryExpr of the form "x + 5".
  /// 
  /// For this reason, if expr() returns a partial nary expression, then
  /// has_aggregate() returns true.
  const SharedExpr expr() const;

  /// Replace concrete data and shared symbolic expression

  /// Assignment operator copies the concrete data and shared symbolic
  /// expression of the other value. Since the assignment usually involves a
  /// temporary value, no self-assignment check is performed.
  ScalarValue& operator=(const ScalarValue& other) {
    __Value<T>::operator=(other);

    m_data = other.m_data;
    m_aggregate = other.m_aggregate;
    m_aggregate_init = other.m_aggregate_init;
    return *this;
  }

  operator T() const { return conv(__id<T>()); }

};

/// Type-safe (symbolic/concrete) rvalue

/// A \ref is_symbolic() "symbolic" value of type T can be created with
/// any(const std::string&).
///
/// In contrast, a \ref is_concrete "concrete" value of type T can be created
/// with make_value(const T). However, such a value object can be still made
/// symbolic by calling set_symbolic(const std::string&).
template<typename T>
class Value : public ScalarValue<T> {
public:

  Value(const T data) : ScalarValue<T>(data) {}
  Value(const T data, const SharedExpr& expr) : ScalarValue<T>(data, expr) {}
  Value(const std::string& identifier) : ScalarValue<T>(identifier) {}
  Value(const Value& other) : ScalarValue<T>(other) {}

  template<typename S>
  Value(const Value<S>& other) : ScalarValue<T>(other) {}

  ~Value() {}
};

template<typename T>
template<typename S>
ScalarValue<T>::ScalarValue(const ScalarValue<S>& other) :
    __Value<T>(other.is_concrete()), m_data(static_cast<T>(other.data())),
    m_aggregate(static_cast<T>(other.aggregate())) {

  if(other.is_symbolic()) {
    const SharedExpr cast_expr(new CastExpr(__Value<T>::type(), other.expr()));
    __Value<T>::set_expr(cast_expr);
  }
}

template<typename T>
T ScalarValue<T>::conv(__id<bool>) const {
  if (__Value<T>::is_symbolic()) {
    if(m_data) {
      tracer().add_path_constraint(__Value<T>::expr());
    } else {
      const SharedExpr unary_expr(new UnaryExpr(NOT, __Value<T>::expr()));
      tracer().add_path_constraint(unary_expr);
    }
  }

  return m_data;
}

template<typename T>
void ScalarValue<T>::set_symbolic(const std::string& identifier) {
  if (__Value<T>::is_concrete()) {
    __Value<T>::set_expr(create_value_expr(identifier));
  } else {
    __Value<T>::set_expr(create_any_expr(identifier));
  }
}

template<typename T>
std::ostream& ScalarValue<T>::write(std::ostream& out) const {
  if (!__Value<T>::is_concrete()) {
    // TODO: Error
  }

  out << m_data;
  return out;
}

template<typename T>
const SharedExpr ScalarValue<T>::expr() const {
  if (__Value<T>::is_symbolic()) {
    SharedExpr raw_expr = __Value<T>::expr();
    if(raw_expr->kind() == NARY_EXPR) {
      std::shared_ptr<NaryExpr> nary_expr = std::dynamic_pointer_cast<NaryExpr>(raw_expr);
      if(nary_expr->is_partial()) {
        if(nary_expr->is_commutative() && nary_expr->is_associative()) {
          if (!has_aggregate()) {
            // TODO: Error
          }

          NaryExpr* new_nary_expr = new NaryExpr(*nary_expr);
          new_nary_expr->append_operand(create_aggregate_expr());
          return SharedExpr(new_nary_expr);
        }
      }
    }
    return raw_expr;
  }

  if (!__Value<T>::is_concrete()) {
    // TODO: Error
  }

  return create_value_expr();
}

/// Create concrete value for multi-path and single-path symbolic execution

/// Create a new value suitable for multi-path (i.e. CBMC-style) and
/// single-path (i.e. DART-style) symbolic execution. The latter is also
/// known as concolic execution.
///
/// \remark Type T should be a primitive type.

// Note: when the returned object is used as a temporary, the compiler is
// likely to use RVO (return value optimization) to avoid the overhead of
// copying the return value.
template<typename T>
inline Value<T> make_value(const T data) {
  return Value<T>(data);
}

/// Create a symbolic value of type T

/// Create an arbitrary value which is only suitable for multi-path
/// (i.e. CBMC-style) symbolic execution. The supplied string corresponds
/// to the name of the newly created \ref AnyExpr "symbolic variable".
template<typename T>
Value<T> any(const std::string& identifier) {
  return Value<T>(identifier);
}

}

#endif /* VALUE_H_ */
