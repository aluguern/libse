#include "concurrent.h"

int main(void) {
  se::init_recorder();

  se::Z3 z3;
  se::MemoryAddrRelation<se::Event> relation;
  const se::Z3ValueEncoder value_encoder;
  const se::Z3OrderEncoder order_encoder;

  se::SharedVar<char> x;
  se::LocalVar<char> a;

  x = 'A';
  if (se::recorder_ptr()->begin_then(x == '?')) {
    x = 'B';
  }
  if (se::recorder_ptr()->begin_else()) {
    x = 'C';
  }
  se::recorder_ptr()->end_branch();
  a = x;

  se::recorder_ptr()->encode(value_encoder, z3);
  se::recorder_ptr()->relate(relation);
  order_encoder.encode(se::recorder_ptr()->most_outer_block_ptr(), relation, z3);

  z3.solver.add(value_encoder.encode(!(a == 'B' || a == 'C'), z3));
  if (z3::unsat == z3.solver.check()) {
    return 0;
  }

  return 1;
}
