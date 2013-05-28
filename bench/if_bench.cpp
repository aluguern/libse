#include "libse.h"

se::Slicer slicer;
se::SharedVar<char> x;

int main(void) {
  slicer.begin_slice_loop();
  do {
    se::Thread::z3().reset();

    se::LocalVar<char> a;

    x = 'A';
    if (slicer.begin_then_branch(__COUNTER__, x == '?')) {
      x = 'B';
    }
    slicer.end_branch(__COUNTER__);
    a = x;

    se::Thread::error(!(a == 'B' || a == 'A'));

    if (se::Thread::encode() && z3::sat == se::Thread::z3().solver.check()) {
      return 1;
    }
  } while (slicer.next_slice());

  return 0;
}
