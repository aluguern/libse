#include <climits>
#include "sp.h"

const int N = 65536;

int main(void) {
  Int k = 0;
  set_symbolic(k);

  Loop loop(N);
  loop.track(k);
  while(loop.unwind(k < INT_MAX)) {
    k = k + 1;
  }

  // force k to be live
  return k != N;
}

