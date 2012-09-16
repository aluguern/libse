#include <climits>
#include <iostream>
#include "sequential-se.h"

using namespace se;

const int N = 8192;

int main(void) {
  // slow down analysis with console I/O to compare against CBMC
  int i = 0;

  Int k = any<int>("K");

  Loop loop(N);
  loop.track(k);
  while(loop.unwind(k < INT_MAX)) {
    k = k + 1;
    std::cout << "Unwinding loop c::main.0 iteration " << i++ << " file while_loop_bench.c line 6 function main" << std::endl;
  }

  // force k to be live
  return k != N;
}

