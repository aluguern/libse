#include "reflect.h"

Tracer& tracer() {
  static Tracer tracer;
  return tracer;
}

