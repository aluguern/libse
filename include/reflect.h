#ifndef REFLECT_H_
#define REFLECT_H_

#include <memory>
#include <string>
#include <ostream>
#include "expr.h"
#include "symtracer.h"

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

// SmartPointer<T> is a helper class that wraps a C++11 std::shared_ptr<T> to
// force the T() and bool() conversion operator to invoke the one of the object
// pointed to by the shared pointer. Since either conversion would fail if the
// wrapped shared pointer points to NULL, this class has no default constructor
// and its only explicit constructor requires the given pointer to be not NULL.
// See also std::shared_ptr<T>
template<typename T>
class SmartPointer {
private:
  std::shared_ptr<T> p;

public:
  // Constructor to manage the given pointer. This pointer must never be NULL.
  explicit SmartPointer(T* ptr) : p(std::shared_ptr<T>(ptr)) {}

  T* operator->() const { return p.operator->(); }
  T& operator*() const { return *p; }
  T* get() const { return p.get(); }

#define CONVERSION(type) \
  operator type() const { return *p; }\

  CONVERSION(bool);
  CONVERSION(char);
  CONVERSION(int);

};

// Forward declaration of a subclass to enable its base class to declare
// functions that the subclass needs to override. This creates strong coupling
// between the subclass and base class but facilitates native type conversions.
template<typename T>
class Value;

// ReflectValue is an abstract base class that represents the concrete and (if
// applicable) symbolic bytes at a specific physical memory address. The type
// information of these bytes is immutable. However, type casts are possible
// and follow the C++ semantics as implemented by the C++ compiler with which
// this library is compiled.
class ReflectValue {
private:
  // type gives the primitive type information of the represented bytes.
  const Type type;

  // expr is a pointer to a symbolic expression that resulted from modifying
  // the symbolic bytes through the overloaded built-in operators. This member
  // is a NULL pointer if and only if the represented bytes are concrete.
  SharedExpr expr;

protected:

  // Constructor that creates the representation of a value of the supplied
  // type. Initially, the represented value is concrete (i.e. not symbolic).
  ReflectValue(const Type type) :
    type(type), expr(SharedExpr()) {}

  // Constructor that creates the representation of a value of the supplied
  // type and shared symbolic expression. The latter forces the represented
  // value to be symbolic.
  ReflectValue(const Type type, const SharedExpr expr) :
    type(type), expr(expr) {}

  // Copy constructor that creates a deep copy of another value representation.
  // Symbolic expressions are shared since these are expected to be immutable.
  ReflectValue(const ReflectValue& other) :
    type(other.type), expr(other.expr) {}

public:

  // Pointer is a shared pointer type that enables polymorphism on reflection
  // values by allowing functions to return an implementation of the abstract
  // ReflectValue class. This pointer supports shared ownership of the pointed
  // to ReflectValue object.
  typedef SmartPointer<ReflectValue> Pointer;

  // Since ReflectValue is a base class, it needs a virtual destructor.
  virtual ~ReflectValue() {}

  // get_type() returns the type of the reflected bytes. The type never changes.
  Type get_type() const { return type; }

  // is_symbolic() returns true if and only if the reflected bytes are symbolic.
  // The bytes are symbolic if and only if get_expr() returns a symbolic
  // expression that is not NULL.
  bool is_symbolic() const { return static_cast<bool>(expr); }

  // set_expr(new_expr) sets the symbolic expression of the reflected bytes.
  // Unless new_expr is a NULL pointer, is_symbolic() returns true afterwards.
  void set_expr(const SharedExpr& new_expr) { expr = new_expr; }

  // get_expr() returns the symbolic expression of the reflected bytes.
  // The return value is a NULL pointer if and only if is_symbolic() is false.
  virtual SharedExpr get_expr() const { return expr; }

  // set_symbolic(const std::string&) marks these reflected bytes as symbolic.
  virtual void set_symbolic(const std::string&) = 0;

  // write(std::ostream&) writes the concrete value of the reflected bytes to
  // the supplied output stream.
  virtual std::ostream& write(std::ostream&) const = 0;

  // TODO: Investigate how to handle type conversions of symbolic expressions.
  virtual ReflectValue& operator=(const ReflectValue& other) {
    expr = other.expr;
    return *this;
  }

// VALUE_API(T) is a macro that defines function signatures which enable the
// safe copying of concrete reflected values whose primitive type is T:
//
// * copy_value<Value<T>&) sets the value in the supplied reflection object
//   to the value encapsulated by the method receiver. For this reason, the
//   function argument is a non-const reference.
// * operator T() const is a const conversion operator which extracts the
//   reflected value as type T.
//
// Since C++ supports implicit primitive type conversions, these conversion
// operators must not be explicit which is a new feature supported by C++11.
//
// In the case T == bool, the bool() conversion operator must add the
// symbolic expression of the reflected bytes to the path constraint if
// and only if is_symbolic() returns true.
#define VALUE_API(type) \
  virtual void copy_value(/* mutable */Value<type>&) const = 0;\
  virtual operator type() const = 0;\

  VALUE_API(bool)
  VALUE_API(char)
  VALUE_API(int)

// VINSTR_TYPE_API is a macro to define the function signature which returns a
// pointer to a newly created reflection value that is the result of performing
// the binary operation op on the receiver and the provided argument. This
// method is supposed to be called in a double dispatch mode. For this reason,
// the operands of the binary operation op arrive in the opposite order!
// For example, b.operator<(a) where both a and b are of type Value<T> must
// compute the strict inequality a < b instead of b < a.
#define VINSTR_TYPE_API(type, op) \
  virtual const Pointer operator op(const Value<type>&) const = 0;\

// VINSTR_API is a macro that must be used to overload a built-in C++ binary
// operator such as addition (+). Since C++ does not support multiple dispatch,
// a double dispatching technique is used to force a separate vtable lookup for
// both operands in the binary operation. This is inefficient but safe. A more
// efficient implementation is possible with template meta-programming in which
// arithmetic conversions are specified in form of templates which must have a
// typedef member whose type is equal to the return type of a binary operation.
// Since C++11 it is also possible to use a new function declaration syntax
// with which we could declare trailing return types of binary operators.
#define VINSTR_API(op) \
  virtual const Pointer operator op(const ReflectValue&) const = 0;\
  VINSTR_TYPE_API(bool, op)\
  VINSTR_TYPE_API(char, op)\
  VINSTR_TYPE_API(int, op)\

  VINSTR_API(+)
  VINSTR_API(<)
};

// reflect(T) creates a pointer to a new reflection value that represents the
// concrete value given as the argument. Type T should be a primitive type.
//
// It is tempting to return directly a ReflectValue object. In fact, when such
// an object is used as a temporary, the compiler is likely to use RVO (return
// value optimization) to avoid the overhead of copying the return value. This
// would be the case in evaluating the overloaded built-in operators. However,
// ReflectValue is an abstract class and thus cannot be instantiated. Since the
// returning of a const reference to a local variable is undefined, we must
// return a pointer to a heap allocated ReflectValue implementation.
template<typename T>
const ReflectValue::Pointer reflect(const T value);

// Value represents the reflection value whose type is of primitive type T.
// Each Value<T> class implements implicit type conversion operators declared
// in the base class. These implementations are standard except from bool()
// which also must add the value's symbolic expression to the path constraints.
template<typename T>
class Value : public ReflectValue {
private:
  // value represents the concrete value of the reflected bytes at a specific
  // physical memory location. The value is mutable unless the Value<T> object
  // has been annotated with the C++ const keyword.
  T value;

  // create_shared_expr() returns a pointer to a value expression that matches
  // the type and content of the private value field.
  SharedExpr create_shared_expr() const;

  // create_shared_expr(const std::string&) returns a pointer to a symbolic
  // value expression that matches the type and content of the private value
  // field. The symbolic variable is named according to the supplied string.
  SharedExpr create_shared_expr(const std::string&) const;

public:
  typedef T type;

  // Default constructor with which the initialization of the concrete and
  // symbolic value (if any) can be deferred. Use with caution and do not
  // publish the instantiated value object until all fields have been set.
  explicit Value() : ReflectValue(ReflectType<T>::type) {}

  // Constructor to create a reflection value based on the supplied concrete
  // value. Initially, the instantiated reflection value is concrete.
  Value(const T value) :
    ReflectValue(ReflectType<T>::type), value(value) {}

  // Constructor to create a reflection value based on the supplied concrete
  // value and symbolic expression. The later forces the instantiated value
  // representation to be also symbolic.
  Value(const T value, const SharedExpr& expr) :
    ReflectValue(ReflectType<T>::type, expr), value(value) {}

  // Copy constructor that creates a reflection value based on another.
  Value(const Value& other) :
    ReflectValue(other), value(other.value) {}

  ~Value() {}

  void set_value(const T new_value) { value = new_value; }
  T get_value() const { return value; }

  virtual void set_symbolic(const std::string&);
  virtual std::ostream& write(std::ostream&) const;
  virtual SharedExpr get_expr() const;

  virtual ReflectValue& operator=(const ReflectValue& other) {
    ReflectValue::operator=(other);

    // use double dispatch
    other.copy_value(*this);
    return *this;
  }

  // See VALUE_API(T) where T == bool
  virtual void copy_value(Value<bool>& __v) const { __v.set_value(value); }
  virtual operator bool() const;

// See VALUE_API(T) where T != bool
#define VALUE(type) \
  virtual void copy_value(Value<type>& __v) const { __v.set_value(value); }\
  virtual operator type() const { return static_cast<type>(value); }\

  VALUE(char)
  VALUE(int)

// See VINSTR_TYPE_API
#define VINSTR_TYPE(type, op, opname) \
  virtual const Pointer operator op(const Value<type>& __v) const {\
    const Pointer& result = reflect(__v.get_value() op value);\
    if(is_symbolic() || __v.is_symbolic()) {\
      result->set_expr(SharedExpr(new BinaryExpr(__v.get_expr(), get_expr(), opname)));\
    }\
    return result;\
  }\

// See VINSTR_API
#define VINSTR(op, opname) \
  virtual const Pointer operator op(const ReflectValue& other) const {\
    return other op *this;\
  }\
  VINSTR_TYPE(bool, op, opname)\
  VINSTR_TYPE(char, op, opname)\
  VINSTR_TYPE(int, op, opname)\

  VINSTR(+, ADD)
  VINSTR(<, LSS)
};

template<typename T>
SharedExpr Value<T>::create_shared_expr() const {
  return SharedExpr(new ValueExpr<type>(value));
}

template<typename T>
SharedExpr Value<T>::create_shared_expr(const std::string& name) const {
  return SharedExpr(new ValueExpr<type>(value, name));
}

template<typename T>
Value<T>::operator bool() const {
  const bool cond = static_cast<bool>(value);
  if(is_symbolic()) {
    if(cond) {
      tracer().add_path_constraint(get_expr());
    } else {
      tracer().add_path_constraint(SharedExpr(new UnaryExpr(get_expr(), NOT)));
    }
  }

  return cond;
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
SharedExpr Value<T>::get_expr() const {
  if(is_symbolic()) {
    return ReflectValue::get_expr();
  }

  return create_shared_expr();
}

template<typename T>
const ReflectValue::Pointer reflect(const T value) {
  return ReflectValue::Pointer(new Value<T>(value));
}

#endif /* REFLECT_H_ */
