#include "gtest/gtest.h"

#include <sstream>

#include "concurrent.h"

using namespace se;

class CharBlockPrinter {
public:
  std::stringstream out;

private:
  int indent;
  void print(std::string str) {
    assert(indent >= 0);
    out << std::string(indent, ' ') << str << std::endl;
  }

  void print(char c) {
    assert(indent >= 0);
    out << std::string(indent, ' ') << c << std::endl;
  }

public:
  CharBlockPrinter() : out(), indent(0) {}

  void print_block_ptr(std::shared_ptr<Block> block_ptr) {
    print("{");
    indent += 2;

    for (std::shared_ptr<Event> event_ptr : block_ptr->body()) {
      if (event_ptr->is_read()) { continue; }

      const WriteEvent<char>& write_event =
        static_cast<const WriteEvent<char>&>(*event_ptr);

      const LiteralReadInstr<char>& literal_read_instr =
        static_cast<const LiteralReadInstr<char>&>(write_event.instr_ref());

      if (literal_read_instr.literal()) {
        print(literal_read_instr.literal());
      }
    }

    for (std::shared_ptr<Block> inner_block_ptr : block_ptr->inner_block_ptrs()) {
      print_block_ptr(inner_block_ptr);
    }

    if (block_ptr->else_block_ptr()) {
      indent -= 2;
      print("} else {");
      indent += 2;

      print_block_ptr(block_ptr->else_block_ptr());
    }

    indent -= 2;
    print("}");
  }
};

TEST(ConcurrentFunctionalTest, Else) {
  Z3C0 z3;
  Slicer slicer;
  CharBlockPrinter printer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  x = 'A';
  if (slicer.begin_then_branch(__COUNTER__, any<bool>())) {
    x = 'B';
  }
  if (slicer.begin_else_branch(__COUNTER__)) {
    x = 'C';
  } slicer.end_branch(__COUNTER__);
  x = 'D';

  printer.print_block_ptr(ThisThread::most_outer_block_ptr());
  EXPECT_EQ("{\n"
            "  {\n"
            "    A\n"
            "  }\n"
            "  {\n"
            "    B\n"
            "  } else {\n"
            "    {\n"
            "      C\n"
            "    }\n"
            "  }\n"
            "  {\n"
            "    D\n"
            "  }\n"
            "}\n", printer.out.str());

  Threads::end_main_thread(z3);
}


TEST(ConcurrentFunctionalTest, ElseIf) {
  Z3C0 z3;
  Slicer slicer;
  CharBlockPrinter printer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  x = 'A';
  slicer.begin_then_branch(__COUNTER__, any<bool>()); {
  } slicer.begin_else_branch(__COUNTER__); {
      slicer.begin_then_branch(__COUNTER__, any<bool>()); {
        slicer.begin_then_branch(__COUNTER__, any<bool>()); {
          x = 'B';
        } slicer.begin_else_branch(__COUNTER__); {
          x = 'C';
          x = 'D';
        } slicer.end_branch(__COUNTER__);
      } slicer.end_branch(__COUNTER__);
      x = 'E';
  } slicer.end_branch(__COUNTER__);

  printer.print_block_ptr(ThisThread::most_outer_block_ptr());
  EXPECT_EQ("{\n"
            "  {\n"
            "    A\n"
            "  }\n"
            "  {\n"
            "  } else {\n"
            "    {\n"
            "      {\n"
            "        {\n"
            "          B\n"
            "        } else {\n"
            "          {\n"
            "            C\n"
            "            D\n"
            "          }\n"
            "        }\n"
            "      }\n"
            "      {\n"
            "        E\n"
            "      }\n"
            "    }\n"
            "  }\n"
            "  {\n  }\n"
            "}\n", printer.out.str());

  Threads::end_main_thread(z3);
}

// Do not introduce any new read events as part of a conditional check
#define TRUE_READ_INSTR \
  (std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)))

#define FALSE_READ_INSTR \
  (std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(false)))

TEST(ConcurrentFunctionalTest, SeriesParallelGraph) {
  Z3C0 z3;
  Slicer slicer;
  CharBlockPrinter printer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  x = 'A';
  slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
    x = 'B';
    slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'C';
      } slicer.end_branch(__COUNTER__);
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'D';
      } slicer.end_branch(__COUNTER__);
    } slicer.end_branch(__COUNTER__);
    slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
      x = 'E';
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'F';
      } slicer.end_branch(__COUNTER__);
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'G';
      } slicer.end_branch(__COUNTER__);
    } slicer.end_branch(__COUNTER__);
    slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'H';
      } slicer.end_branch(__COUNTER__);
      x = 'I';
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'J';
      } slicer.end_branch(__COUNTER__);
    } slicer.end_branch(__COUNTER__);
    slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
      x = 'K';
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'L';
      } slicer.end_branch(__COUNTER__);
      x = 'M';
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'N';
      } slicer.end_branch(__COUNTER__);
    } slicer.end_branch(__COUNTER__);
  } slicer.begin_else_branch(__COUNTER__); {
    slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'O';
      } slicer.begin_else_branch(__COUNTER__); {
        x = 'P';
      } slicer.end_branch(__COUNTER__);
    } slicer.end_branch(__COUNTER__);
    x = 'Q';
    slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
      x = 'R';
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'S';
      } slicer.begin_else_branch(__COUNTER__); {
        x = 'T';
      } slicer.end_branch(__COUNTER__);
    } slicer.end_branch(__COUNTER__);
    slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'U';
      } slicer.begin_else_branch(__COUNTER__); {
        x = 'W';
      } slicer.end_branch(__COUNTER__);
      x = 'V';
    } slicer.end_branch(__COUNTER__);
    slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
      x = 'X';
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'Y';
      } slicer.end_branch(__COUNTER__);
      slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR); {
        x = 'Z';
      } slicer.end_branch(__COUNTER__);
      x = '!';
    } slicer.end_branch(__COUNTER__);
  } slicer.end_branch(__COUNTER__);

  printer.print_block_ptr(ThisThread::most_outer_block_ptr());
  EXPECT_EQ("{\n"
            "  {\n"
            "    A\n"
            "  }\n"
            "  {\n"
            "    B\n"
            "    {\n"
            "      {\n"
            "        C\n"
            "      }\n"
            "      {\n"
            "        D\n"
            "      }\n"
            "    }\n"
            "    {\n"
            "      E\n"
            "      {\n"
            "        F\n"
            "      }\n"
            "      {\n"
            "        G\n"
            "      }\n"
            "    }\n"
            "    {\n"
            "      {\n"
            "        H\n"
            "      }\n"
            "      {\n"
            "        I\n"
            "      }\n"
            "      {\n"
            "        J\n"
            "      }\n"
            "    }\n"
            "    {\n"
            "      K\n"
            "      {\n"
            "        L\n"
            "      }\n"
            "      {\n"
            "        M\n"
            "      }\n"
            "      {\n"
            "        N\n"
            "      }\n"
            "    }\n"
            "  } else {\n"
            "    {\n"
            "      {\n"
            "        {\n"
            "          O\n"
            "        } else {\n"
            "          {\n"
            "            P\n"
            "          }\n"
            "        }\n"
            "      }\n"
            "      {\n"
            "        Q\n"
            "      }\n"
            "      {\n"
            "        R\n"
            "        {\n"
            "          S\n"
            "        } else {\n"
            "          {\n"
            "            T\n"
            "          }\n"
            "        }\n"
            "      }\n"
            "      {\n"
            "        {\n"
            "          U\n"
            "        } else {\n"
            "          {\n"
            "            W\n"
            "          }\n"
            "        }\n"
            "        {\n"
            "          V\n"
            "        }\n"
            "      }\n"
            "      {\n"
            "        X\n"
            "        {\n"
            "          Y\n"
            "        }\n"
            "        {\n"
            "          Z\n"
            "        }\n"
            "        {\n"
            "          !\n"
            "        }\n"
            "      }\n"
            "    }\n"
            "  }\n"
            "  {\n  }\n"
            "}\n", printer.out.str());

  Threads::end_main_thread(z3);
}

/*
TEST(ConcurrentFunctionalTest, Loop) {
  Z3C0 z3;
  Slicer slicer;
  CharBlockPrinter printer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  x = 'A';
  char k = 1;
  while (ThisThread::recorder().unwind_loop(x < '?', make_loop_policy<__COUNTER__, 3>())) {
    x = k++;
  }

  printer.print_block_ptr(ThisThread::most_outer_block_ptr());
  EXPECT_EQ("{\n"
            "  {\n"
            "    A\n"
            "  }\n"
            "  {\n"
            "    \x1\n"
            "    {\n"
            "      \x2\n"
            "      {\n"
            "        \x3\n"
            "      }\n"
            "    }\n"
            "  }\n"
            "  {\n  }\n"
            "}\n", printer.out.str());

  Threads::end_main_thread(z3);
}
*/

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

TEST(ConcurrentFunctionalTest, SatSlicerZeroInSingleThreadWithSharedVar) {
  Z3C0 z3;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  if (slicer.begin_then_branch(__COUNTER__, any<bool>())) {
    x = 'B';
  }
  if (slicer.begin_else_branch(__COUNTER__)) {
    x = 'C';
  }
  slicer.end_branch(__COUNTER__);
  a = x;

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(a == 'B');
  std::unique_ptr<ReadInstr<bool>> c1(!(a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c2(a == 'B' && !(a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c3(a == 'C');
  std::unique_ptr<ReadInstr<bool>> c4(!(a == 'B') && a == 'C');
  std::unique_ptr<ReadInstr<bool>> c5(a == 'A');

  Threads::end_main_thread(z3);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(z3::sat, z3.solver.check());

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
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, UnsatSlicerZeroInSingleThreadWithSharedVar) {
  Z3C0 z3;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  if (slicer.begin_then_branch(__COUNTER__, x == '?')) {
    x = 'B';
  }
  if (slicer.begin_else_branch(__COUNTER__)) {
    x = 'C';
  }
  slicer.end_branch(__COUNTER__);
  a = x;

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(!(a == 'B' || a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c1(a == 'A');

  Threads::end_main_thread(z3);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

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


TEST(ConcurrentFunctionalTest, SatSlicerZeroInSingleThreadWithNondetermisticConditionAndArraySharedVar) {
  Z3C0 z3;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[3]> x;
  SharedVar<char> y;
  LocalVar<char> a;

  x[2] = 'A';
  if (slicer.begin_then_branch(__COUNTER__, any<bool>())) {
    x[2] = 'B';
  }
  if (slicer.begin_else_branch(__COUNTER__)) {
    x[2] = 'C';
  }
  slicer.end_branch(__COUNTER__);
  a = x[2];

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(a == 'B');
  std::unique_ptr<ReadInstr<bool>> c1(!(a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c2(a == 'B' && !(a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c3(!(a == 'B') && a == 'C');

  Threads::end_main_thread(z3);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(z3::sat, z3.solver.check());

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
}

TEST(ConcurrentFunctionalTest, SatSlicerZeroInSingleThreadWithDeterminsticConditionAndArraySharedVar) {
  Z3C0 z3;
  Slicer slicer; 

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[3]> x;
  LocalVar<char> p;
  LocalVar<char> a;

  p = '?';
  x[2] = 'A';
  if (slicer.begin_then_branch(__COUNTER__, p == '?')) {
    x[2] = 'B'; // choose always 'B' branch
  }
  if (slicer.begin_else_branch(__COUNTER__)) {
    x[2] = 'C';
  }
  slicer.end_branch(__COUNTER__);
  a = x[2];

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(a == 'B');
  std::unique_ptr<ReadInstr<bool>> c1(!(a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c3(!(a == 'B') && a == 'C');

  Threads::end_main_thread(z3);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c3), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, UnsatSlicerZeroInSingleThreadWithArraySharedVar) {
  Z3C0 z3;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[3]> x;
  LocalVar<char> a;

  a = 'X';
  x[2] = 'A';
  if (slicer.begin_then_branch(__COUNTER__, a == '?')) {
    x[2] = 'B';
  }
  if (slicer.begin_else_branch(__COUNTER__)) {
    x[2] = 'C';
  }
  slicer.end_branch(__COUNTER__);
  a = x[2];

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(!(a == 'B' || a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c1(a == 'A');

  Threads::end_main_thread(z3);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

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

TEST(ConcurrentFunctionalTest, UnsatSlicerZeroConditionalErrorSingleThread) {
  Z3C0 z3;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> var;
  var = any<int>();

  if (slicer.begin_then_branch(__COUNTER__, 0 < var)) {
    Threads::error(var == 0, z3);
  }
  slicer.end_branch(__COUNTER__);
  Threads::end_main_thread(z3);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, UnsatSlicerZeroConditionalErrorMultipleThreads) {
  Z3C0 z3;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> var;
  var = any<int>();

  Threads::begin_thread();

  if (slicer.begin_then_branch(__COUNTER__, 0 < var)) {
    Threads::error(var == 0, z3);
  }
  slicer.end_branch(__COUNTER__);

  Threads::end_thread();
  Threads::end_main_thread(z3);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, UnsatSlicerZeroFalseConditionalError) {
  Z3C0 z3;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  Threads::begin_thread();
  if (slicer.begin_then_branch(__COUNTER__, FALSE_READ_INSTR)) {
    Threads::error(TRUE_READ_INSTR, z3);
  }
  slicer.end_branch(__COUNTER__);
  Threads::end_thread();

  Threads::end_main_thread(z3);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, SatSlicerZeroErrorAmongAnotherUnsatError) {
  Z3C0 z3;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  if (slicer.begin_then_branch(__COUNTER__, FALSE_READ_INSTR)) {
    Threads::error(TRUE_READ_INSTR, z3);
  }
  slicer.end_branch(__COUNTER__);

  if (slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR)) {
    Threads::error(TRUE_READ_INSTR, z3);
  }
  slicer.end_branch(__COUNTER__);

  Threads::end_main_thread(z3);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(z3::sat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, SatSlicerMaxInSingleThreadWithSharedVar) {
  Slicer slicer(MAX_SLICE_FREQ);

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
    if (slicer.begin_then_branch(__COUNTER__, any<bool>())) {
      x = 'B';
    }
    if (slicer.begin_else_branch(__COUNTER__)) {
      x = 'C';
    }
    slicer.end_branch(__COUNTER__);
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

TEST(ConcurrentFunctionalTest, SatSlicerMaxInSingleThreadWithNondetermisticConditionAndArraySharedVar) {
  Slicer slicer(MAX_SLICE_FREQ);

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
    if (slicer.begin_then_branch(__COUNTER__, any<bool>())) {
      x[2] = 'B';
    }
    if (slicer.begin_else_branch(__COUNTER__)) {
      x[2] = 'C';
    }
    slicer.end_branch(__COUNTER__);
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

TEST(ConcurrentFunctionalTest, SatSlicerMaxInSingleThreadWithDeterminsticConditionAndArraySharedVar) {
  Slicer slicer(MAX_SLICE_FREQ);

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
    if (slicer.begin_then_branch(__COUNTER__, p == '?')) {
      x[2] = 'B'; // choose always 'B' branch
    }
    if (slicer.begin_else_branch(__COUNTER__)) {
      x[2] = 'C';
    }
    slicer.end_branch(__COUNTER__);
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

TEST(ConcurrentFunctionalTest, UnsatSlicerMaxConditionalErrorSingleThread) {
  Slicer slicer(MAX_SLICE_FREQ);

  unsigned checks = 0;
  unsigned unchecks = 0;

  do {
    Z3C0 z3;

    Threads::reset();
    Threads::begin_main_thread();

    SharedVar<int> var;
    var = any<int>();

    if (slicer.begin_then_branch(__COUNTER__, 0 < var)) {
      Threads::error(var == 0, z3);
    }
    slicer.end_branch(__COUNTER__);

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

TEST(ConcurrentFunctionalTest, UnsatSlicerMaxConditionalErrorMultipleThreads) {
  Slicer slicer(MAX_SLICE_FREQ);

  unsigned checks = 0;
  unsigned unchecks = 0;

  do {
    Z3C0 z3;

    Threads::reset();
    Threads::begin_main_thread();

    SharedVar<int> var;
    var = any<int>();

    Threads::begin_thread();

    if (slicer.begin_then_branch(__COUNTER__, 0 < var)) {
      Threads::error(var == 0, z3);
    }
    slicer.end_branch(__COUNTER__);

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

TEST(ConcurrentFunctionalTest, UnsatSlicerMaxFalseConditionalError) {
  Slicer slicer(MAX_SLICE_FREQ);

  unsigned checks = 0;
  unsigned unchecks = 0;

  do {
    Z3C0 z3;

    Threads::reset();
    Threads::begin_main_thread();

    Threads::begin_thread();
    if (slicer.begin_then_branch(__COUNTER__, FALSE_READ_INSTR)) {
      Threads::error(TRUE_READ_INSTR, z3);
    }
    slicer.end_branch(__COUNTER__);
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

TEST(ConcurrentFunctionalTest, SatSlicerMaxErrorAmongAnotherUnsatError) {
  Slicer slicer(MAX_SLICE_FREQ);

  unsigned sat_checks = 0;
  unsigned unsat_checks = 0;
  unsigned unknown_checks = 0;
  unsigned unchecks = 0;
  z3::check_result expect = z3::unknown;

  do {
    Z3C0 z3;

    Threads::reset();
    Threads::begin_main_thread();

    if (slicer.begin_then_branch(__COUNTER__, FALSE_READ_INSTR)) {
      expect = z3::unsat;
      Threads::error(TRUE_READ_INSTR, z3);
    }
    slicer.end_branch(__COUNTER__);

    if (slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR)) {
      expect = z3::sat;
      Threads::error(TRUE_READ_INSTR, z3);
    }
    slicer.end_branch(__COUNTER__);

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
