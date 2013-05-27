#include "gtest/gtest.h"

#include <sstream>

#include "concurrent.h"

using namespace se;

TEST(ConcurrentFunctionalTest, Counter) {
  constexpr unsigned counter = __COUNTER__;
  EXPECT_EQ(counter + 1, __COUNTER__);
}

/*
TEST(ConcurrentFunctionalTest, LocalArray) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  Threads::begin_thread();

  LocalVar<char[5]> a;
  a[2] = 'Z';

  Threads::error(!(a[2] == 'Z'), z3);

  // Do not encode global memory accesses for this test
  Threads::end_thread();

  std::stringstream out;
  out << z3.solver;
  EXPECT_EQ("(solver\n  (= k!2 ((as const (Array (_ BitVec 64) (_ BitVec 8))) #x00))\n"
            "  (= k!3 (store k!2 #x0000000000000002 #x5a))\n"
            "  true\n  (> k!1 0)\n  (< epoch k!1)\n"
            "  (> k!4 0)\n  (< k!1 k!4))", out.str());

  // error condition has not been added yet
  EXPECT_EQ(z3::sat, z3.solver.check());

  Threads::end_main_thread(z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());
}
*/

// Do not introduce any new read events as part of a conditional check
#define TRUE_READ_INSTR \
  (std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)))

#define FALSE_READ_INSTR \
  (std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(false)))

TEST(ConcurrentFunctionalTest, ThreeThreadsReadWriteScalarSharedVar) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  // Declare shared variable initialized by main thread
  SharedVar<char> x;

  // Record first child thread
  Threads::begin_thread();

  x = 'P';

  Threads::end_thread();

  // Record second child thread
  Threads::begin_thread();

  x = 'Q';

  Threads::end_thread();

  // Prepare property encoding
  LocalVar<char> a;
  a = x;

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(!(a == '\0'));
  std::unique_ptr<ReadInstr<bool>> c1(!(a == 'P'));
  std::unique_ptr<ReadInstr<bool>> c2(!(a == 'Q'));
  std::unique_ptr<ReadInstr<bool>> c3(!(a == 'P' || a == 'Q'));
  std::unique_ptr<ReadInstr<bool>> c4(!(a == '\0' || a == 'P'));
  std::unique_ptr<ReadInstr<bool>> c5(!(a == '\0' || a == 'Q'));
  std::unique_ptr<ReadInstr<bool>> c6(!(a == '\0' || a == 'P' || a == 'Q'));

  Threads::end_main_thread(z3);

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c2), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c3), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c4), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c5), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c6), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, SatSingleThreadWithSharedVar) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  a = x;

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::sat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, UnsatInSingleThreadWithSharedVar) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  x = 'B';
  a = x;

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(a == 'A');

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, SatSlicesInSingleThreadWithSharedVar) {
  Slicer slicer;

  bool r0 = false;
  bool r1 = false;
  bool r2 = false;
  bool r3 = false;
  bool r4 = false;

  do {
    Z3C0 z3;

    Threads::reset();
    Threads::begin_main_thread();

    SharedVar<char> x;
    LocalVar<char> a;

    x = 'A';
    if (slicer.begin_block(__COUNTER__, any<bool>())) {
      x = 'B';
    } else {
      x = 'C';
    }
    slicer.end_block(__COUNTER__);
    a = x;

    // Create properties within main thread context
    std::unique_ptr<ReadInstr<bool>> c0(a == 'B');
    std::unique_ptr<ReadInstr<bool>> c1(!(a == 'C'));
    std::unique_ptr<ReadInstr<bool>> c2(a == 'B' && !(a == 'C'));
    std::unique_ptr<ReadInstr<bool>> c3(a == 'C');
    std::unique_ptr<ReadInstr<bool>> c4(!(a == 'B') && a == 'C');

    Threads::end_main_thread(z3);

    EXPECT_EQ(z3::sat, z3.solver.check());

    z3.solver.push();

    Threads::internal_error(std::move(c0), z3);
    r0 |= z3::sat == z3.solver.check();

    z3.solver.pop();

    z3.solver.push();

    Threads::internal_error(std::move(c1), z3);
    r1 |= z3::sat == z3.solver.check();

    z3.solver.pop();

    z3.solver.push();

    Threads::internal_error(std::move(c2), z3);
    r2 |= z3::sat == z3.solver.check();

    z3.solver.pop();

    z3.solver.push();

    Threads::internal_error(std::move(c3), z3);
    r3 |= z3::sat == z3.solver.check();

    z3.solver.pop();

    z3.solver.push();

    Threads::internal_error(std::move(c4), z3);
    r4 |= z3::sat == z3.solver.check();

    z3.solver.pop();
  } while (slicer.next_slice());

  EXPECT_EQ(2, slicer.slice_count());

  EXPECT_TRUE(r0);
  EXPECT_TRUE(r1);
  EXPECT_TRUE(r2);
  EXPECT_TRUE(r3);
  EXPECT_TRUE(r4);
}

/*

TEST(ConcurrentFunctionalTest, UnsatSlicesInSingleThreadWithSharedVar) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  if (ThisThread::recorder().begin_then(x == '?')) {
    x = 'B';
  }
  if (ThisThread::recorder().begin_else()) {
    x = 'C';
  }
  ThisThread::recorder().end_branch();
  a = x;

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(!(a == 'B' || a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c1(a == 'A');

  Threads::end_main_thread(z3);

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}
*/

TEST(ConcurrentFunctionalTest, LocalScalarAndLocalArray) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<char[5]> a;
  LocalVar<char> b;
  a[2] = 'Z';
  b = a[2];

  std::unique_ptr<ReadInstr<bool>> c0(!(b == 'Z'));

  // Do not encode global memory accesses for this test
  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, SharedScalarVariableInSingleThread) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> a;
  LocalVar<char> b;
  a = 'Z';
  b = a;

  std::unique_ptr<ReadInstr<bool>> c0((b == 'Z'));
  std::unique_ptr<ReadInstr<bool>> c1(!(b == 'Z'));

  Threads::end_main_thread(z3);

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, SharedArrayInSingleThread) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[5]> a;
  LocalVar<char> b;
  a[2] = 'Z';
  b = a[2];

  std::unique_ptr<ReadInstr<bool>> c0(!(b == 'Z'));

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, SharedArrayWithSymbolicIndex) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[3]> xs;
  SharedVar<size_t> index = 1;
  LocalVar<char> a;

  xs[index] = 'Y';
 
  index = index + static_cast<size_t>(1);
  xs[index] = 'Z';

  a = xs[0];
  std::unique_ptr<ReadInstr<bool>> c0(a == 'Z');
  std::unique_ptr<ReadInstr<bool>> c1(!(a == 'Z'));

  a = xs[1];
  std::unique_ptr<ReadInstr<bool>> c2(a == 'Z');
  std::unique_ptr<ReadInstr<bool>> c3(!(a == 'Z'));

  a = xs[2];
  std::unique_ptr<ReadInstr<bool>> c4(a == 'Z');
  std::unique_ptr<ReadInstr<bool>> c5(!(a == 'Z'));

  a = xs[index];
  std::unique_ptr<ReadInstr<bool>> c6(a == 'Z');
  std::unique_ptr<ReadInstr<bool>> c7(!(a == 'Z'));

  // no overflow yet
  index = index + static_cast<size_t>(1);
  a = xs[index];
  std::unique_ptr<ReadInstr<bool>> c8(a == 'Z');
  std::unique_ptr<ReadInstr<bool>> c9(!(a == 'Z'));

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c2), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c3), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c4), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c5), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c6), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c7), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c8), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c9), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, MultipleWritesToSharedArrayLocationsInSingleThreadButLiteralIndex) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[3]> xs;
  LocalVar<char> v;

  xs[0] = 'A';
  xs[1] = 'B';

  v = xs[1];

  se::Threads::error(!('B' == v), z3);
  Threads::end_main_thread(z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, MultipleWritesToSharedArrayInSingleThreadAndVariableIndex) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[3]> xs;
  LocalVar<char> v;
  SharedVar<size_t> index;

  index = 0;
  xs[index] = 'A';

  index = index + static_cast<size_t>(1);
  xs[index] = 'B';

  v = xs[index];

  se::Threads::error(!('B' == v), z3);
  Threads::end_main_thread(z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());
}


TEST(ConcurrentFunctionalTest, SatSlicesInSingleThreadWithNondetermisticConditionAndArraySharedVar) {
  Slicer slicer;

  bool r0 = false;
  bool r1 = false;
  bool r2 = false;
  bool r3 = false;

  do {
    Z3C0 z3;
  
    Threads::reset();
    Threads::begin_main_thread();
  
    SharedVar<char[3]> x;
    SharedVar<char> y;
    LocalVar<char> a;
  
    x[2] = 'A';
    if (slicer.begin_block(__COUNTER__, any<bool>())) {
      x[2] = 'B';
    } else {
      x[2] = 'C';
    }
    slicer.end_block(__COUNTER__);
    a = x[2];
  
    // Create properties within main thread context
    std::unique_ptr<ReadInstr<bool>> c0(a == 'B');
    std::unique_ptr<ReadInstr<bool>> c1(!(a == 'C'));
    std::unique_ptr<ReadInstr<bool>> c2(a == 'B' && !(a == 'C'));
    std::unique_ptr<ReadInstr<bool>> c3(!(a == 'B') && a == 'C');
  
    Threads::end_main_thread(z3);
  
    EXPECT_EQ(z3::sat, z3.solver.check());
  
    z3.solver.push();
  
    Threads::internal_error(std::move(c0), z3);
    r0 |= z3::sat == z3.solver.check();
  
    z3.solver.pop();
  
    z3.solver.push();
  
    Threads::internal_error(std::move(c1), z3);
    r1 |= z3::sat == z3.solver.check();
  
    z3.solver.pop();
  
    z3.solver.push();
  
    Threads::internal_error(std::move(c2), z3);
    r2 |= z3::sat == z3.solver.check();
  
    z3.solver.pop();
  
    z3.solver.push();
  
    Threads::internal_error(std::move(c3), z3);
    r3 |= z3::sat == z3.solver.check();
  
    z3.solver.pop();
  } while (slicer.next_slice());

  EXPECT_EQ(2, slicer.slice_count());

  EXPECT_TRUE(r0);
  EXPECT_TRUE(r1);
  EXPECT_TRUE(r2);
  EXPECT_TRUE(r3);
}

TEST(ConcurrentFunctionalTest, SatSlicesInSingleThreadWithDeterminsticConditionAndArraySharedVar) {
  Slicer slicer; 

  bool r0 = false;
  bool r1 = false;
  bool r2 = true;

  do {
    Z3C0 z3;
  
    Threads::reset();
    Threads::begin_main_thread();
  
    SharedVar<char[3]> x;
    LocalVar<char> p;
    LocalVar<char> a;
  
    p = '?';
    x[2] = 'A';
    if (slicer.begin_block(__COUNTER__, p == '?')) {
      x[2] = 'B'; // choose always 'B' branch
    } else {
      x[2] = 'C';
    }
    slicer.end_block(__COUNTER__);
    a = x[2];
  
    // Create properties within main thread context
    std::unique_ptr<ReadInstr<bool>> c0(a == 'B');
    std::unique_ptr<ReadInstr<bool>> c1(!(a == 'C'));
    std::unique_ptr<ReadInstr<bool>> c2(!(a == 'B') && a == 'C');
  
    Threads::end_main_thread(z3);
  
    EXPECT_EQ(z3::sat, z3.solver.check());
  
    z3.solver.push();
  
    Threads::internal_error(std::move(c0), z3);
    r0 |= z3::sat == z3.solver.check();
  
    z3.solver.pop();
  
    z3.solver.push();
  
    Threads::internal_error(std::move(c1), z3);
    r1 |= z3::sat == z3.solver.check();
  
    z3.solver.pop();
  
    z3.solver.push();
  
    Threads::internal_error(std::move(c2), z3);
    r2 &= z3::unsat == z3.solver.check();
  
    z3.solver.pop();
  } while (slicer.next_slice());

  EXPECT_EQ(2, slicer.slice_count());

  EXPECT_TRUE(r0);
  EXPECT_TRUE(r1);
  EXPECT_TRUE(r2);
}

/*
TEST(ConcurrentFunctionalTest, UnsatSlicesInSingleThreadWithArraySharedVar) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[3]> x;
  LocalVar<char> a;

  a = 'X';
  x[2] = 'A';
  if (ThisThread::recorder().begin_then(a == '?')) {
    x[2] = 'B';
  }
  if (ThisThread::recorder().begin_else()) {
    x[2] = 'C';
  }
  ThisThread::recorder().end_branch();
  a = x[2];

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(!(a == 'B' || a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c1(a == 'A');

  Threads::end_main_thread(z3);

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}
*/

TEST(ConcurrentFunctionalTest, SatJoinThreads) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  Threads::begin_thread();

  x = 'A';

  Threads::end_thread();

  Threads::error(x == '\0', z3);

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::sat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, UnsatJoinThreads) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  Threads::begin_thread();

  x = 'A';

  const std::shared_ptr<SendEvent> send_event_ptr = Threads::end_thread();

  Threads::join(send_event_ptr);
  Threads::error(x == '\0', z3);

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, CopyLocalVar) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<char> a = 'A';
  LocalVar<char> b = a;

  std::unique_ptr<ReadInstr<bool>> c0(!(b == 'A'));
  std::unique_ptr<ReadInstr<bool>> c1(!(b == a));

  a = 'B';
  std::unique_ptr<ReadInstr<bool>> c2(!(b == a));

  Threads::end_main_thread(z3);

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c2), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, CopySharedVarToLocalVar) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> a = 'A';
  LocalVar<char> b = a;

  std::unique_ptr<ReadInstr<bool>> c0(b == 'A');
  std::unique_ptr<ReadInstr<bool>> c1(!(b == 'A'));

  a = 'B';
  std::unique_ptr<ReadInstr<bool>> c2(!(b == 'A'));

  Threads::end_main_thread(z3);

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c2), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, AnyReadInstr) {
  Z3C0 z3;

  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<char> a = 'A';

  a = any<char>();

  std::unique_ptr<ReadInstr<bool>> c0(a == '*');
  std::unique_ptr<ReadInstr<bool>> c1(!(a == '*'));

  a = '*';
  std::unique_ptr<ReadInstr<bool>> c2(a == '*');
  std::unique_ptr<ReadInstr<bool>> c3(!(a == '*'));

  Threads::end_main_thread(z3);

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c2), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c3), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, ConditionalErrorSingleThread) {
  Slicer slicer;

  unsigned checks = 0;
  unsigned unchecks = 0;

  do {
    Z3C0 z3;

    Threads::reset();
    Threads::begin_main_thread();

    SharedVar<int> var;
    var = any<int>();

    if (slicer.begin_block(__COUNTER__, 0 < var)) {
      Threads::error(var == 0, z3);
    }
    slicer.end_block(__COUNTER__);

    const bool check = Threads::end_main_thread(z3);
    if (check) {
      checks++;
      EXPECT_EQ(z3::unsat, z3.solver.check());
    } else {
      unchecks++;
    }
  } while (slicer.next_slice());

  EXPECT_EQ(2, slicer.slice_count());

  EXPECT_EQ(1, checks);
  EXPECT_EQ(1, unchecks);
}


TEST(ConcurrentFunctionalTest, ConditionalErrorMultipleThreads) {
  Slicer slicer;

  unsigned checks = 0;
  unsigned unchecks = 0;

  do {
    Z3C0 z3;

    Threads::reset();
    Threads::begin_main_thread();

    SharedVar<int> var;
    var = any<int>();

    Threads::begin_thread();

    if (slicer.begin_block(__COUNTER__, 0 < var)) {
      Threads::error(var == 0, z3);
    }
    slicer.end_block(__COUNTER__);

    Threads::end_thread();

    const bool check = Threads::end_main_thread(z3);
    if (check) {
      checks++;
      EXPECT_EQ(z3::unsat, z3.solver.check());
    } else {
      unchecks++;
    }
  } while (slicer.next_slice());

  EXPECT_EQ(2, slicer.slice_count());

  EXPECT_EQ(1, checks);
  EXPECT_EQ(1, unchecks);
}


TEST(ConcurrentFunctionalTest, FalseConditionalError) {
  Slicer slicer;

  unsigned checks = 0;
  unsigned unchecks = 0;

  do {
    Z3C0 z3;

    Threads::reset();
    Threads::begin_main_thread();

    Threads::begin_thread();
    if (slicer.begin_block(__COUNTER__, FALSE_READ_INSTR)) {
      Threads::error(TRUE_READ_INSTR, z3);
    }
    slicer.end_block(__COUNTER__);
    Threads::end_thread();

    const bool check = Threads::end_main_thread(z3);
    if (check) {
      checks++;
      EXPECT_EQ(z3::unsat, z3.solver.check());
    } else {
      unchecks++;
    }
  } while (slicer.next_slice());

  EXPECT_EQ(2, slicer.slice_count());

  EXPECT_EQ(1, checks);
  EXPECT_EQ(1, unchecks);
}

TEST(ConcurrentFunctionalTest, SatErrorAmongAnotherUnsatError) {
  Slicer slicer;

  unsigned sat_checks = 0;
  unsigned unsat_checks = 0;
  unsigned unknown_checks = 0;
  unsigned unchecks = 0;
  z3::check_result expect = z3::unknown;

  do {
    Z3C0 z3;

    Threads::reset();
    Threads::begin_main_thread();

    if (slicer.begin_block(__COUNTER__, FALSE_READ_INSTR)) {
      expect = z3::unsat;
      Threads::error(TRUE_READ_INSTR, z3);
    }
    slicer.end_block(__COUNTER__);

    if (slicer.begin_block(__COUNTER__, TRUE_READ_INSTR)) {
      expect = z3::sat;
      Threads::error(TRUE_READ_INSTR, z3);
    }
    slicer.end_block(__COUNTER__);

    const bool check = Threads::end_main_thread(z3);
    if (check) {
      switch (expect) {
      case z3::sat:     sat_checks++;     break;
      case z3::unsat:   unsat_checks++;   break;
      case z3::unknown: unknown_checks++; break;
      }
      EXPECT_EQ(expect, z3.solver.check());
    } else {
      unchecks++;
    }
    expect = z3::unknown;
  } while (slicer.next_slice());

  EXPECT_EQ(4, slicer.slice_count());

  EXPECT_EQ(2, sat_checks);
  EXPECT_EQ(1, unsat_checks);
  EXPECT_EQ(0, unknown_checks);
  EXPECT_EQ(1, unchecks);
}
