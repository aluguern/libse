#include "concurrent.h"

int main(void) {
  se::Z3 z3;

  se::Threads::reset();
  se::Threads::begin_main_thread();

  se::SharedVar<char> x;
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

  se::Threads::error(!(a == 'B' || a == 'C'), z3);
  se::Threads::end_main_thread(z3);

  return z3::sat == z3.solver.check();
}
