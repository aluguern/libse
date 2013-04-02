// Adapted from the SV-COMP'13 benchmark:
//   https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/fib_bench_unsafe.c

#include "libse.h"

#define N 5

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
  se::Thread t0(f0);
  se::Thread t1(f1);

  se::Thread::error(144 < i || 144 == i || 144 < j || 144 == j);

  t0.join();
  t1.join();

  se::Thread::end_main_thread();

  return z3::unsat == se::Thread::z3().solver.check();
}
