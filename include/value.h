#ifndef VALUE_H_
#define VALUE_H_

#include <memory>
#include <string>
#include <ostream>
#include "expr.h"
#include "tracer.h"

// tracer() returns a static object that can record path constraints.
extern Tracer& tracer();

// ReflectType is a lookup function that maps primitive types to an enum Type.
// Since template specializations are used, this lookup occurs at compile-time.
template<typename T>
class ReflectType {};

#define REFLECT_TYPE(reflect_type, builtin_type)\
template<>\
class ReflectType<builtin_type> {\
  public:\
    static const Type type = reflect_type;\
};\

REFLECT_TYPE(BOOL, bool)
REFLECT_TYPE(CHAR, char)
REFLECT_TYPE(INT, int)

// GenericValue is an abstract base class that represents the concrete and (if
// applicable) symbolic value at a specific physical memory address. Concrete
// values are never shared and must be therefore copied by using memcpy or the
// value's copy constructor. Symbolic expressions (if any) are shared until one
// is modified. The type information of a GenericValue object is immutable.
// However, type casts are possible and follow the C++11 language semantics as
// implemented by the C++ compiler with which this library is compiled. In the
// case when type casts were needed and the GenericValue object represents a
// symbolic variable (i.e. is_symbolic() returns true), its current symbolic
// expression is going to be wrapped inside a new CastExpr.
class GenericValue {
private:
  // type is an immutable field that gives the primitive type information of
  // the represented concrete and (if applicable) symbolic value.
  const Type type;

  // expr is a shared pointer to a symbolic expression that captures changes to
  // the symbolic value through the overloaded built-in operators. This field
  // is a NULL pointer if and only if the represented value is concrete.
  SharedExpr expr;

protected:

  // Constructor that creates the representation of a value of the supplied
  // type. Initially, the represented value is concrete (i.e. not symbolic).
  GenericValue(const Type type) :
    type(type), expr(SharedExpr()) {}

  // Constructor that creates the representation of a value of the supplied
  // type and shared symbolic expression. The latter forces the represented
  // value to be symbolic.
  GenericValue(const Type type, const SharedExpr expr) :
    type(type), expr(expr) {}

  // Copy constructor that creates a deep copy of another value representation.
  // Symbolic expressions are shared since these are expected to be immutable.
  GenericValue(const GenericValue& other) :
    type(other.type), expr(other.expr) {}

  // Assignment operator that assigns the symbolic expression (if any) of the
  // supplied argument to this object. Since GenericValue assignments usually
  // involve one temporary object, no self-assignment check is performed.
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

  // set_expr(new_expr) sets the symbolic expression of the reflected value.
  // Unless new_expr is a NULL pointer, is_symbolic() returns true afterwards.
  void set_expr(const SharedExpr& new_expr) { expr = new_expr; }

  // get_expr() returns the symbolic expression of the reflected value.
  // The return value is a NULL pointer if and only if is_symbolic() is false.
  virtual const SharedExpr get_expr() const { return expr; }

  // set_symbolic(const std::string&) marks this reflected value as symbolic.
  virtual void set_symbolic(const std::string&) = 0;

  // write(std::ostream&) writes the concrete value of the reflected value to
  // the supplied output stream.
  virtual std::ostream& write(std::ostream&) const = 0;

};

// __any and __id are internal helper structures used for static compile-time
// overloading of conversion operators. More specifically, it enables finding
// the type of a template parameter. If its type does not matter, use __any.
struct __any {};
template <typename T> struct __id : __any { typedef T type; };

// Value represents the reflection value whose type is of primitive type T.
// Each Value<T> class implements an implicit type conversion operator. This
// conversion operator is standard except for bool() which also must add the
// value's symbolic expression to the path constraints. Currently, the use of
// non-bool types in control flow statements is unsupported (e.g. if(5) {...}).
template<typename T>
class Value : public GenericValue {
private:
  // value represents the concrete value of the reflected value at a specific
  // physical memory location. The value is mutable unless the Value<T> object
  // has been annotated with the C++ const keyword.
  T value;

  // create_shared_expr() returns a shared pointer to a value expression that
  // matches the type and content of the private value field.
  SharedExpr create_shared_expr() const;

  // create_shared_expr(const std::string&) returns a pointer to a symbolic
  // value expression that matches the type and content of the private value
  // field. The symbolic variable is named according to the supplied string.
  SharedExpr create_shared_expr(const std::string&) const;

  // conv(__any) is a default conversion function that returns the primitive
  // value of type T.
  T conv(__any) const { return value; }

  // conv(__id<bool>) overloads conv(__any). It adds the symbolic expression to
  // the path constraints if and only if is_symbolic() returns true.
  T conv(__id<bool>) const;

public:
  typedef T type;

  // Constructor to create a reflection value based on the supplied concrete
  // value. Initially, the instantiated reflection value is concrete.
  Value(const T value) :
    GenericValue(ReflectType<T>::type), value(value) {}

  // Constructor to create a reflection value based on the supplied concrete
  // value and symbolic expression. The later forces the instantiated value
  // representation to be also symbolic.
  Value(const T value, const SharedExpr& expr) :
    GenericValue(ReflectType<T>::type, expr), value(value) {}

  // Copy constructor that creates a reflection value based on another.
  Value(const Value& other) : GenericValue(other), value(other.value) {}

  // Copy conversion constructor that creates a reflection value based on
  // another whose type is different from the template parameter T. Thus,
  // a type conversion is required which could result in precision loss.
  // If the supplied value is symbolic, the symbolic expression of the new
  // value is going to be wrapped inside a CastExpr.
  template<typename S>
  Value(const Value<S>&);

  virtual ~Value() {}

  // set_value(new_value) sets the concrete value of this value representation.
  void set_value(const T new_value) { value = new_value; }

  // get_value() returns the concrete value of this value representation.
  T get_value() const { return value; }

  // Overrides GenericValue::set_symbolic(const std::string&)
  virtual void set_symbolic(const std::string&);

  // Overrides GenericValue::write(std::outstream&)
  virtual std::ostream& write(std::ostream&) const;

  // Overrides GenericValue::get_expr()
  virtual const SharedExpr get_expr() const;

  // Assignment operator that copies the concrete value and shared symbolic
  // expression of the supplied value representation. Since the assignment
  // usually involves a temporary value, no self-assignment check is performed.
  Value& operator=(const Value& other) {
    GenericValue::operator=(other);

    value = other.value;
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
  GenericValue(ReflectType<T>::type), value(other.get_value()) {
  if(other.is_symbolic()) {
    set_expr(SharedExpr(new CastExpr(other.get_expr(), get_type())));
  }
}

template<typename T>
SharedExpr Value<T>::create_shared_expr() const {
  return SharedExpr(new ValueExpr<type>(value));
}

template<typename T>
SharedExpr Value<T>::create_shared_expr(const std::string& name) const {
  return SharedExpr(new ValueExpr<type>(value, name));
}

template<typename T>
T Value<T>::conv(__id<bool>) const {
  if(is_symbolic()) {
    if(value) {
      tracer().add_path_constraint(get_expr());
    } else {
      tracer().add_path_constraint(SharedExpr(new UnaryExpr(get_expr(), NOT)));
    }
  }

  return value;
}

template<typename T>
void Value<T>::set_symbolic(const std::string& name) {
  if(!is_symbolic()) {
    set_expr(create_shared_expr(name));
  }
}

template<typename T>
std::ostream& Value<T>::write(std::ostream& out) const {
  out << value;
  return out;
}

template<typename T>
const SharedExpr Value<T>::get_expr() const {
  if(is_symbolic()) {
    return GenericValue::get_expr();
  }

  return create_shared_expr();
}

// reflect(T) creates reflection value that represents the concrete value given
// as the argument. Type T should be a primitive type. When the returned object
// is used as a temporary, the compiler is likely to use RVO (return value
// optimization) to avoid the overhead of copying the return value.
template<typename T>
inline const Value<T> reflect(const T value) {
  return Value<T>(value);
}

#endif /* VALUE_H_ */
