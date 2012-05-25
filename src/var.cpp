#include "var.h"

namespace sp {

unsigned int var_seq = 0;

void reset_tracer() {
  tracer().reset();
  var_seq = 0;
}

}

