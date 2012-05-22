#include "value.h"

Tracer& tracer() {
  static Tracer tracer;
  return tracer;
}

