// Adapted from the SV-COMP'13 benchmark:
//   https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/fib_bench_safe.c

#include "concurrent.h"

#define NUM 5

int main(void) {
  se::Z3 z3;

  se::Threads::reset();
  se::Threads::begin_main_thread();

  se::SharedVar<int> i = 1, j = 1;

  int k;

  se::Threads::begin_thread(/* T0 */);
  for (k = 0; k < NUM; k++) {
    i = i + j;
  }
  se::Threads::end_thread(z3);

  se::Threads::begin_thread(/* T1 */);
  for (k = 0; k < NUM; k++) {
    j = j + i;
  }
  se::Threads::end_thread(z3);

  se::Threads::error(144 < i || 144 < j, z3);
  se::Threads::end_main_thread(z3);

  return z3::sat == z3.solver.check();
}
