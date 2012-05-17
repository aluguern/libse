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
// primitive type. An object must be guaranteed to be instantiated only once.
#define ANY_DECL(type) \
extern Value<type> any_##type();

ANY_DECL(bool)
ANY_DECL(char)
ANY_DECL(int)

#endif /* ANY_H_ */
