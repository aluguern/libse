#include <iostream>
#include "sp.h"

/* 2^16 */
const int N = 65536;

int main(void) {
  // slow down analysis with console I/O to compare against CBMC
  int i = 0;

  Int k = 0;
  for(Int n = 0; n < N; n = n + 1) {
    if(k == 7) {
      k = 0;
    }
    std::cout << "Unwinding loop c::main.0 iteration " << i++ << " file explicit_loop_bench.c line 6 function main" << std::endl;
    k = k + 1;
  }

  // force k to be live
  return k != N % 7;
}

