// Adapted from the SV-COMP'13 benchmark:
//   https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/stack_safe.c

#include "libse.h"
#include "concurrent/mutex.h"

#define N 12

se::SharedVar<unsigned int> top = 0;
se::SharedVar<int> flag = 0;
se::Mutex mutex;

void push(int x) {
  se::Thread::error(top == N);

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
    flag = 1;
    mutex.unlock();
  }
}

void f2() {
  int i;
  for (i = 0; i < N; i++) {
    mutex.lock();
    if (se::ThisThread::recorder().begin_then(flag == 1)) {
      pop();
    }
    se::ThisThread::recorder().end_branch();
    mutex.unlock();
  }
}

int main(void) {
  se::Thread t1(f1);
  se::Thread t2(f2);

  se::Thread::end_main_thread();

  return z3::sat == se::Thread::z3().solver.check();
}
