#include "value.h"

namespace se {

Tracer& tracer() {
  static Tracer tracer;
  return tracer;
}

}

