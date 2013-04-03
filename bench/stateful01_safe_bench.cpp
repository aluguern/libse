// Adapted from the SV-COMP'13 benchmark:
//   https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/stateful01_safe.c

#include "libse.h"
#include "concurrent/mutex.h"

se::SharedVar<int> i = 10, j = 10;
se::Mutex mutex;

void f0() {
  mutex.lock();
  i = i + 1;
  mutex.unlock();

  mutex.lock();
  j = j + 1;
  mutex.unlock();
}

void f1() {
  mutex.lock();
  i = i + 5;
  mutex.unlock();

  mutex.lock();
  j = j - 6;
  mutex.unlock();
}

int main(void) {
  se::Thread t0(f0);
  se::Thread t1(f1);

  t0.join();
  t1.join();

  se::Thread::error(!(i == 16) || !(j == 5));

  se::Thread::end_main_thread();

  return z3::sat == se::Thread::z3().solver.check();
}
