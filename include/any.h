#ifndef ANY_H_
#define ANY_H_

#include "value.h"

namespace se {

// Internal class that represents an arbitrary value of primitive type T.
// It should be only instantiated as a singleton object.
template<typename T>
class AnyValue : public Value<T> {

private:

  // create_shared_expr(const std::string&) returns a shared pointer to an
  // AnyExpr<T> object.
  SharedExpr create_shared_expr(const std::string&) const;

public:

  // Overrides Value<T>::set_symbolic(const std::string&)
  virtual void set_symbolic(const std::string&);

  AnyValue() : Value<T>(0) {}
  ~AnyValue() {}

};

template<typename T>
SharedExpr AnyValue<T>::create_shared_expr(const std::string& name) const {
  return SharedExpr(new AnyExpr<T>(name));
}

template<typename T>
void AnyValue<T>::set_symbolic(const std::string& name) {
  if(!GenericValue::is_symbolic()) {
    GenericValue::set_expr(create_shared_expr(name));
  }
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

#endif /* ANY_H_ */
