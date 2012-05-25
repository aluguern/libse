#include "value.h"

namespace sp {

Tracer& tracer() {
  static Tracer tracer;
  return tracer;
}

}

