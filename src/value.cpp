#include "value.h"

namespace se {

Tracer& tracer() {
  static Tracer tracer;
  return tracer;
}

// Macro to define a function which has been declared with ANY_DECL.
#define ANY_DEF(type) \
Value<type> any_##type(const std::string& name) {\
  return Value<type>(name);\
}

ANY_DEF(bool)
ANY_DEF(char)
ANY_DEF(int)

}

