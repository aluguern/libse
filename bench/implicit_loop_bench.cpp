#include <climits>
#include "overload.h"

const int N = 65536;

int main(void) {
  Int k = 0;
  set_symbolic(k);

  Loop loop(N);
  while(loop.unwind(k < INT_MAX)) {
    loop.begin_loop(k);

    k = k + 1;

    loop.end_loop(k);
  }
  loop.join(k);

  // force k to be live
  return k != N;
}

