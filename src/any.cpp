#include "any.h"

// Macro to define a function which has been declared with ANY_DECL.
#define ANY_DEF(type) \
Value<type> any_##type(const std::string& name) {\
  AnyValue<type> any;\
  any.set_symbolic(name);\
  return any;\
}

ANY_DEF(bool)
ANY_DEF(char)
ANY_DEF(int)

