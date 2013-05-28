// Adapted from the SV-COMP'13 benchmark:
//   https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/stack_safe.c

#include "libse.h"
#include "concurrent/mutex.h"

#define N 12

se::Slicer slicer;
se::SharedVar<unsigned int> top = 0;
se::Mutex mutex;

void push(int x) {
  top = top + 1;
}

int pop() {
  se::Thread::error(top == 0);

  top = top - 1;
  return 0;
}

void f1() {
  int i;
  for (i = 0; i < N; i++) {
    mutex.lock();
    push(i);
    mutex.unlock();
  }
}

void f2() {
  int i;
  for (i = 0; i < N; i++) {
    mutex.lock();
    if (slicer.begin_then_branch(__COUNTER__, 0 < top)) {
      pop();
    }
    slicer.end_branch(__COUNTER__);
    mutex.unlock();
  }
}

int main(void) {
  slicer.begin_slice_loop();
  do {
    se::Thread::z3().reset();

    se::Thread t1(f1);
    se::Thread t2(f2);

    if (se::Thread::encode() && z3::sat == se::Thread::z3().solver.check()) {
      return 1;
    }
  } while (slicer.next_slice());

  return 0;
}
