#include <sstream>

#include "concurrent.h"
#include "gtest/gtest.h"

using namespace se;
using namespace se::ops;

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
  Encoders encoders;
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

  Threads::end_main_thread(encoders);
}


TEST(ConcurrentFunctionalTest, ElseIf) {
  Encoders encoders;
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

  Threads::end_main_thread(encoders);
}

// Do not introduce any new read events as part of a conditional check
#define TRUE_READ_INSTR \
  (std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)))

#define FALSE_READ_INSTR \
  (std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(false)))

TEST(ConcurrentFunctionalTest, SeriesParallelGraph) {
  Encoders encoders;
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

  Threads::end_main_thread(encoders);
}

/*
TEST(ConcurrentFunctionalTest, Loop) {
  Encoders encoders;
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

  Threads::end_main_thread(encoders);
}
*/

TEST(ConcurrentFunctionalTest, Counter) {
  constexpr unsigned counter = __COUNTER__;
  EXPECT_EQ(counter + 1, __COUNTER__);
}

/*
TEST(ConcurrentFunctionalTest, LocalArray) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  Threads::begin_thread();

  LocalVar<char[5]> a;
  a[2] = 'Z';

  Threads::error(!(a[2] == 'Z'), encoders);

  // Do not encode global memory accesses for this test
  Threads::end_thread();

  std::stringstream out;
  out << encoders.solver;
  EXPECT_EQ("(solver\n  (= k!2 ((as const (Array (_ BitVec 64) (_ BitVec 8))) #x00))\n"
            "  (= k!3 (store k!2 #x0000000000000002 #x5a))\n"
            "  true\n  (> k!1 0)\n  (< epoch k!1)\n"
            "  (> k!4 0)\n  (< k!1 k!4))", out.str());

  // error condition has not been added yet
  EXPECT_EQ(smt::sat, encoders.solver.check());

  Threads::end_main_thread(encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}
*/

// Do not introduce any new read events as part of a conditional check
#define TRUE_READ_INSTR \
  (std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)))

#define FALSE_READ_INSTR \
  (std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(false)))

TEST(ConcurrentFunctionalTest, ThreeThreadsReadWriteScalarSharedVar) {
  Encoders encoders;

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

  Threads::end_main_thread(encoders);

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c2), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c3), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c4), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c5), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c6), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, SatSingleThreadWithSharedVar) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  a = x;

  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::sat, encoders.solver.check());
}

TEST(ConcurrentFunctionalTest, UnsatInSingleThreadWithSharedVar) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  x = 'B';
  a = x;

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(a == 'A');

  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, SatSlicerZeroInSingleThreadWithSharedVar) {
  Encoders encoders;
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

  Threads::end_main_thread(encoders);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c2), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c3), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c4), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c5), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, UnsatSlicerZeroInSingleThreadWithSharedVar) {
  Encoders encoders;
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

  Threads::end_main_thread(encoders);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, LocalScalarAndLocalArray) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<char[5]> a;
  LocalVar<char> b;
  a[2] = 'Z';
  b = a[2];

  std::unique_ptr<ReadInstr<bool>> c0(!(b == 'Z'));

  // Do not encode global memory accesses for this test
  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(ConcurrentFunctionalTest, SharedScalarVariableInSingleThread) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> a;
  LocalVar<char> b;
  a = 'Z';
  b = a;

  std::unique_ptr<ReadInstr<bool>> c0((b == 'Z'));
  std::unique_ptr<ReadInstr<bool>> c1(!(b == 'Z'));

  Threads::end_main_thread(encoders);

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, SharedArrayInSingleThread) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[5]> a;
  LocalVar<char> b;
  a[2] = 'Z';
  b = a[2];

  std::unique_ptr<ReadInstr<bool>> c0(!(b == 'Z'));

  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, SharedArrayWithSymbolicIndex) {
  Encoders encoders;

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

  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c2), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c3), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c4), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c5), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c6), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c7), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  // Out of bound array access does not cause an error.
  //  Threads::internal_error(std::move(c8), encoders);
  //  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c9), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, MultipleWritesToSharedArrayLocationsInSingleThreadButLiteralIndex) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[3]> xs;
  LocalVar<char> v;

  xs[0] = 'A';
  xs[1] = 'B';

  v = xs[1];

  se::Threads::error(!('B' == v), encoders);
  Threads::end_main_thread(encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(ConcurrentFunctionalTest, MultipleWritesToSharedArrayInSingleThreadAndVariableIndex) {
  Encoders encoders;

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

  se::Threads::error(!('B' == v), encoders);
  Threads::end_main_thread(encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}


TEST(ConcurrentFunctionalTest, SatSlicerZeroInSingleThreadWithNondetermisticConditionAndArraySharedVar) {
  Encoders encoders;
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

  Threads::end_main_thread(encoders);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c2), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c3), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, SatSlicerZeroInSingleThreadWithDeterminsticConditionAndArraySharedVar) {
  Encoders encoders;
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

  Threads::end_main_thread(encoders);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c3), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, UnsatSlicerZeroInSingleThreadWithArraySharedVar) {
  Encoders encoders;
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

  Threads::end_main_thread(encoders);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, SatJoinThreads) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  Threads::begin_thread();

  x = 'A';

  Threads::end_thread();

  Threads::error(x == '\0', encoders);

  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::sat, encoders.solver.check());
}

TEST(ConcurrentFunctionalTest, UnsatJoinThreads) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  Threads::begin_thread();

  x = 'A';

  const std::shared_ptr<SendEvent> send_event_ptr = Threads::end_thread();

  Threads::join(send_event_ptr);
  Threads::error(x == '\0', encoders);

  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(ConcurrentFunctionalTest, CopyLocalVar) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<char> a = 'A';
  LocalVar<char> b = a;

  std::unique_ptr<ReadInstr<bool>> c0(!(b == 'A'));
  std::unique_ptr<ReadInstr<bool>> c1(!(b == a));

  a = 'B';
  std::unique_ptr<ReadInstr<bool>> c2(!(b == a));

  Threads::end_main_thread(encoders);

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c2), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, CopySharedVarToLocalVar) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> a = 'A';
  LocalVar<char> b = a;

  std::unique_ptr<ReadInstr<bool>> c0(b == 'A');
  std::unique_ptr<ReadInstr<bool>> c1(!(b == 'A'));

  a = 'B';
  std::unique_ptr<ReadInstr<bool>> c2(!(b == 'A'));

  Threads::end_main_thread(encoders);

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c2), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, AnyReadInstr) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<char> a = 'A';

  a = any<char>();

  std::unique_ptr<ReadInstr<bool>> c0(a == '*');
  std::unique_ptr<ReadInstr<bool>> c1(!(a == '*'));

  a = '*';
  std::unique_ptr<ReadInstr<bool>> c2(a == '*');
  std::unique_ptr<ReadInstr<bool>> c3(!(a == '*'));

  Threads::end_main_thread(encoders);

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c2), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c3), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(ConcurrentFunctionalTest, UnsatSlicerZeroConditionalErrorSingleThread) {
  Encoders encoders;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> var;
  var = any<int>();

  if (slicer.begin_then_branch(__COUNTER__, 0 < var)) {
    Threads::error(var == 0, encoders);
  }
  slicer.end_branch(__COUNTER__);
  Threads::end_main_thread(encoders);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(ConcurrentFunctionalTest, UnsatSlicerZeroConditionalErrorMultipleThreads) {
  Encoders encoders;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> var;
  var = any<int>();

  Threads::begin_thread();

  if (slicer.begin_then_branch(__COUNTER__, 0 < var)) {
    Threads::error(var == 0, encoders);
  }
  slicer.end_branch(__COUNTER__);

  Threads::end_thread();
  Threads::end_main_thread(encoders);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(ConcurrentFunctionalTest, UnsatSlicerZeroFalseConditionalError) {
  Encoders encoders;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  Threads::begin_thread();
  if (slicer.begin_then_branch(__COUNTER__, FALSE_READ_INSTR)) {
    Threads::error(TRUE_READ_INSTR, encoders);
  }
  slicer.end_branch(__COUNTER__);
  Threads::end_thread();

  Threads::end_main_thread(encoders);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(ConcurrentFunctionalTest, SatSlicerZeroErrorAmongAnotherUnsatError) {
  Encoders encoders;
  Slicer slicer;

  Threads::reset();
  Threads::begin_main_thread();

  if (slicer.begin_then_branch(__COUNTER__, FALSE_READ_INSTR)) {
    Threads::error(TRUE_READ_INSTR, encoders);
  }
  slicer.end_branch(__COUNTER__);

  if (slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR)) {
    Threads::error(TRUE_READ_INSTR, encoders);
  }
  slicer.end_branch(__COUNTER__);

  Threads::end_main_thread(encoders);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(1, slicer.slice_count());
  EXPECT_EQ(smt::sat, encoders.solver.check());
}

TEST(ConcurrentFunctionalTest, SatSlicerMaxInSingleThreadWithSharedVar) {
  Slicer slicer(MAX_SLICE_FREQ);

  bool r0 = false;
  bool r1 = false;
  bool r2 = false;
  bool r3 = false;
  bool r4 = false;

  do {
    Encoders encoders;

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

    Threads::end_main_thread(encoders);

    EXPECT_EQ(smt::sat, encoders.solver.check());

    encoders.solver.push();

    Threads::internal_error(std::move(c0), encoders);
    r0 |= smt::sat == encoders.solver.check();

    encoders.solver.pop();

    encoders.solver.push();

    Threads::internal_error(std::move(c1), encoders);
    r1 |= smt::sat == encoders.solver.check();

    encoders.solver.pop();

    encoders.solver.push();

    Threads::internal_error(std::move(c2), encoders);
    r2 |= smt::sat == encoders.solver.check();

    encoders.solver.pop();

    encoders.solver.push();

    Threads::internal_error(std::move(c3), encoders);
    r3 |= smt::sat == encoders.solver.check();

    encoders.solver.pop();

    encoders.solver.push();

    Threads::internal_error(std::move(c4), encoders);
    r4 |= smt::sat == encoders.solver.check();

    encoders.solver.pop();
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
    Encoders encoders;
  
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
  
    Threads::end_main_thread(encoders);
  
    EXPECT_EQ(smt::sat, encoders.solver.check());
  
    encoders.solver.push();
  
    Threads::internal_error(std::move(c0), encoders);
    r0 |= smt::sat == encoders.solver.check();
  
    encoders.solver.pop();
  
    encoders.solver.push();
  
    Threads::internal_error(std::move(c1), encoders);
    r1 |= smt::sat == encoders.solver.check();
  
    encoders.solver.pop();
  
    encoders.solver.push();
  
    Threads::internal_error(std::move(c2), encoders);
    r2 |= smt::sat == encoders.solver.check();
  
    encoders.solver.pop();
  
    encoders.solver.push();
  
    Threads::internal_error(std::move(c3), encoders);
    r3 |= smt::sat == encoders.solver.check();
  
    encoders.solver.pop();
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
    Encoders encoders;
  
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
  
    Threads::end_main_thread(encoders);
  
    EXPECT_EQ(smt::sat, encoders.solver.check());
  
    encoders.solver.push();
  
    Threads::internal_error(std::move(c0), encoders);
    r0 |= smt::sat == encoders.solver.check();
  
    encoders.solver.pop();
  
    encoders.solver.push();
  
    Threads::internal_error(std::move(c1), encoders);
    r1 |= smt::sat == encoders.solver.check();
  
    encoders.solver.pop();
  
    encoders.solver.push();
  
    Threads::internal_error(std::move(c2), encoders);
    r2 &= smt::unsat == encoders.solver.check();
  
    encoders.solver.pop();
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
    Encoders encoders;

    Threads::reset();
    Threads::begin_main_thread();

    SharedVar<int> var;
    var = any<int>();

    if (slicer.begin_then_branch(__COUNTER__, 0 < var)) {
      Threads::error(var == 0, encoders);
    }
    slicer.end_branch(__COUNTER__);

    const bool check = Threads::end_main_thread(encoders);
    if (check) {
      checks++;
      EXPECT_EQ(smt::unsat, encoders.solver.check());
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
    Encoders encoders;

    Threads::reset();
    Threads::begin_main_thread();

    SharedVar<int> var;
    var = any<int>();

    Threads::begin_thread();

    if (slicer.begin_then_branch(__COUNTER__, 0 < var)) {
      Threads::error(var == 0, encoders);
    }
    slicer.end_branch(__COUNTER__);

    Threads::end_thread();

    const bool check = Threads::end_main_thread(encoders);
    if (check) {
      checks++;
      EXPECT_EQ(smt::unsat, encoders.solver.check());
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
    Encoders encoders;

    Threads::reset();
    Threads::begin_main_thread();

    Threads::begin_thread();
    if (slicer.begin_then_branch(__COUNTER__, FALSE_READ_INSTR)) {
      Threads::error(TRUE_READ_INSTR, encoders);
    }
    slicer.end_branch(__COUNTER__);
    Threads::end_thread();

    const bool check = Threads::end_main_thread(encoders);
    if (check) {
      checks++;
      EXPECT_EQ(smt::unsat, encoders.solver.check());
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
  smt::CheckResult expect = smt::unknown;

  do {
    Encoders encoders;

    Threads::reset();
    Threads::begin_main_thread();

    if (slicer.begin_then_branch(__COUNTER__, FALSE_READ_INSTR)) {
      expect = smt::unsat;
      Threads::error(TRUE_READ_INSTR, encoders);
    }
    slicer.end_branch(__COUNTER__);

    if (slicer.begin_then_branch(__COUNTER__, TRUE_READ_INSTR)) {
      expect = smt::sat;
      Threads::error(TRUE_READ_INSTR, encoders);
    }
    slicer.end_branch(__COUNTER__);

    const bool check = Threads::end_main_thread(encoders);
    if (check) {
      switch (expect) {
      case smt::sat:     sat_checks++;     break;
      case smt::unsat:   unsat_checks++;   break;
      case smt::unknown: unknown_checks++; break;
      }
      EXPECT_EQ(expect, encoders.solver.check());
    } else {
      unchecks++;
    }
    expect = smt::unknown;
  } while (slicer.next_slice());

  EXPECT_EQ(4, slicer.slice_count());

  EXPECT_EQ(2, sat_checks);
  EXPECT_EQ(1, unsat_checks);
  EXPECT_EQ(0, unknown_checks);
  EXPECT_EQ(1, unchecks);
}
