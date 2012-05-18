#ifndef ANY_H_
#define ANY_H_

#include "reflect.h"

// Internal class that represents an arbitrary value of primitive type T.
// It should be only instantiated as a singleton object.
template<typename T>
class AnyValue : public Value<T> {

public:

  AnyValue() : Value<T>(0) {}
  ~AnyValue() {}

};

// Macro to declare a function that returns an arbitrary value of the specified
// primitive type. A new object is guaranteed to be instantiated on each call.
// The argument corresponds to the name of the newly created symbolic variable.
#define ANY_DECL(type) \
extern Value<type> any_##type(const std::string& name);

ANY_DECL(bool)
ANY_DECL(char)
ANY_DECL(int)

#endif /* ANY_H_ */
