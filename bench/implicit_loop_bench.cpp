#include <climits>
#include "sequential-se.h"

const int N = 65536;

int main(void) {
  se::Int k = 0;
  se::set_symbolic(k);

  se::Loop loop(N);
  loop.track(k);
  while(loop.unwind(k < INT_MAX)) {
    k = k + 1;
  }

  // force k to be live
  return k != N;
}

