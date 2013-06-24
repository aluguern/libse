// Adapted from the SV-COMP'13 benchmark:
//   https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/fib_bench_longer_unsafe.c

#include "libse.h"

using namespace se::ops;

#define N 8

se::Slicer slicer;
se::SharedVar<int> i = 1, j = 1;

void f0() {
  int k;
  for (k = 0; k < N; k++) {
    i = i + j;
  }
}

void f1() {
  int k;
  for (k = 0; k < N; k++) {
    j = j + i;
  }
}

int main(void) {
  slicer.begin_slice_loop();
  do {
    se::Thread::encoders().reset();

    se::Thread t0(f0);
    se::Thread t1(f1);

    se::Thread::error(2584 < i || 2584 == i || 2584 < j || 2584 == j);

    t0.join();
    t1.join();

    if (se::Thread::encode() && smt::sat == se::Thread::encoders().solver.check()) {
      return 0;
    }
  } while (slicer.next_slice());

  return 1;
}
