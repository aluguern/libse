#include "any.h"

// Macro to define a function which has been declared with ANY_DECL. Each
// function uses an internal static variable to instantiate a singleton object.
#define ANY_DEF(type) \
Value<type> any_##type() {\
  static AnyValue<type> any_##type;\
  return any_##type;\
}

ANY_DEF(bool)
ANY_DEF(char)
ANY_DEF(int)

