#include "libse.h"

se::SharedVar<char> x;

int main(void) {
  se::LocalVar<char> a;

  x = 'A';
  if (se::ThisThread::recorder().begin_then(x == '?')) {
    x = 'B';
  }
  if (se::ThisThread::recorder().begin_else()) {
    x = 'C';
  }
  se::ThisThread::recorder().end_branch();
  a = x;

  se::Thread::error(!(a == 'B' || a == 'C'));
  se::Thread::end_main_thread();

  return z3::sat == se::Thread::z3().solver.check();
}
