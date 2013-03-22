#include "gtest/gtest.h"

#include <sstream>

#include "concurrent.h"

using namespace se;

TEST(ConcurrentFunctionalTest, LocalArray) {
  // Setup
  Event::reset_id();
  init_recorder();

  Z3 z3;
  MemoryAddrRelation<Event> relation;
  const Z3ValueEncoder value_encoder;
  const Z3OrderEncoder order_encoder;

  LocalVar<char[5]> a;
  a[2] = 'Z';

  recorder_ptr()->encode(value_encoder, z3);
  z3.solver.add(value_encoder.encode(!(a[2] == 'Z'), z3));
  EXPECT_EQ(z3::unsat, z3.solver.check());

  std::stringstream out;
  out << z3.solver;
  EXPECT_EQ("(solver\n  (= k!3 (store k!1 #x0000000000000002 #x5a))\n"
            "  (= k!1 ((as const (Array (_ BitVec 64) (_ BitVec 8))) #x00))\n"
            "  (not (= (select k!3 #x0000000000000002) #x5a)))", out.str());
}

TEST(ConcurrentFunctionalTest, LocalScalarAndArray) {
  // Setup
  Event::reset_id();
  init_recorder();

  Z3 z3;
  MemoryAddrRelation<Event> relation;
  const Z3ValueEncoder value_encoder;

  LocalVar<char[5]> a;
  LocalVar<char> b;
  a[2] = 'Z';
  b = a[2];

  recorder_ptr()->encode(value_encoder, z3);
  z3.solver.add(value_encoder.encode(!(b == 'Z'), z3));
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, ThreeThreadsReadWriteScalarSharedVar) {
  // Setup
  Event::reset_id();
  init_recorder();

  Z3 z3;
  MemoryAddrRelation<Event> relation;
  const Z3ValueEncoder value_encoder;
  const Z3OrderEncoder order_encoder;

  // Declare shared variable initialized by main thread
  SharedVar<char> x;

  // Record first child thread
  push_recorder(1);

  x = 'P';

  recorder_ptr()->encode(value_encoder, z3);
  recorder_ptr()->relate(relation);
  pop_recorder();

  // Record second child thread
  push_recorder(2);

  x = '\0';

  recorder_ptr()->encode(value_encoder, z3);
  recorder_ptr()->relate(relation);
  pop_recorder();

  // Prepare property encoding
  LocalVar<char> a;
  a = x;

  recorder_ptr()->encode(value_encoder, z3);
  recorder_ptr()->relate(relation);

  // Encode partial orders
  order_encoder.encode(relation, z3);

  z3.solver.push();

  z3.solver.add(value_encoder.encode(!(a == 'P'), z3));
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(value_encoder.encode(!(a == 'P' || a == '\0'), z3));
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}
