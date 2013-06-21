// Adapted from the SV-COMP'13 benchmark:
//   https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/stack_safe.c

#include "libse.h"
#include "concurrent/mutex.h"

using namespace se::ops;

#define N 12

se::Slicer slicer;
se::SharedVar<unsigned int> top = 0U;
se::SharedVar<int> flag = 0;
se::Mutex mutex;

void push(int x) {
  top = top + 1U;
}

int pop() {
  se::Thread::error(top == 0U);

  top = top - 1U;
  return 0;
}

void f1() {
  int i;
  for (i = 0; i < N; i++) {
    mutex.lock();
    push(i);
    flag = 1;
    mutex.unlock();
  }
}

void f2() {
  int i;
  for (i = 0; i < N; i++) {
    mutex.lock();
    if (slicer.begin_then_branch(__COUNTER__, flag == 1)) {
      pop();
    }
    slicer.end_branch(__COUNTER__);
    mutex.unlock();
  }
}

int main(void) {
  slicer.begin_slice_loop();
  do {
    se::Thread::encoders().reset();

    se::Thread t1(f1);
    se::Thread t2(f2);

    if (se::Thread::encode() && smt::sat == se::Thread::encoders().solver.check()) {
      return 0;
    }
  } while (slicer.next_slice());

  return 1;
}
