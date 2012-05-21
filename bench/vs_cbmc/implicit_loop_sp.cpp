#include <climits>
#include <iostream>
#include "overload.h"

const int N = 65536;

int main(void) {
  // slow down analysis with console I/O to compare against CBMC
  int i = 0;

  Int k = any_int("K");

  Loop loop(N);
  while(loop.unwind(k < INT_MAX)) {
    loop.begin_loop(k);

    k = k + 1;
    std::cout << "Unwinding loop c::main.0 iteration " << i++ << " file implicit_loop_bench.c line 8 function main" << std::endl;

    loop.end_loop(k);
  }
  loop.join(k);

  // force k to be live
  return k != N;
}

