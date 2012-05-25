#include <climits>
#include "sp.h"

const int N = 65536;

int main(void) {
  sp::Int k = 0;
  sp::set_symbolic(k);

  sp::Loop loop(N);
  loop.track(k);
  while(loop.unwind(k < INT_MAX)) {
    k = k + 1;
  }

  // force k to be live
  return k != N;
}

