#include "gtest/gtest.h"

#include <sstream>

#include "concurrent.h"

using namespace se;

TEST(ConcurrentFunctionalTest, Counter) {
  constexpr unsigned counter = __COUNTER__;
  EXPECT_EQ(counter + 1, __COUNTER__);
}

TEST(ConcurrentFunctionalTest, LocalArray) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  LocalVar<char[5]> a;
  a[2] = 'Z';

  Threads::error(!(a[2] == 'Z'), z3);

  // Do not encode global memory accesses for this test
  Threads::end_thread(z3);

  EXPECT_EQ(z3::unsat, z3.solver.check());

  std::stringstream out;
  out << z3.solver;
  EXPECT_EQ("(solver\n  (not (= (select k!3 #x0000000000000002) #x5a))\n"
            "  (= k!1 ((as const (Array (_ BitVec 64) (_ BitVec 8))) #x00))\n"
            "  (= k!3 (store k!1 #x0000000000000002 #x5a))\n"
            "  true\n  (> k!5 0)\n  (< epoch_clock k!5))", out.str());
}

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
  CharBlockPrinter printer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  x = 'A';
  if (ThisThread::recorder().begin_then(any<bool>())) {
    x = 'B';
  }
  if (ThisThread::recorder().begin_else()) {
    x = 'C';
  } ThisThread::recorder().end_branch();
  x = 'D';

  printer.print_block_ptr(ThisThread::recorder().most_outer_block_ptr());
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
}


TEST(ConcurrentFunctionalTest, ElseIf) {
  CharBlockPrinter printer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  x = 'A';
  ThisThread::recorder().begin_then(any<bool>()); {
  } ThisThread::recorder().begin_else(); {
      ThisThread::recorder().begin_then(any<bool>()); {
        ThisThread::recorder().begin_then(any<bool>()); {
          x = 'B';
        } ThisThread::recorder().begin_else(); {
          x = 'C';
          x = 'D';
        } ThisThread::recorder().end_branch();
      } ThisThread::recorder().end_branch();
      x = 'E';
  } ThisThread::recorder().end_branch();

  printer.print_block_ptr(ThisThread::recorder().most_outer_block_ptr());
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
}

// Do not introduce any new read events as part of a conditional check
#define TRUE_READ_INSTR \
  (std::unique_ptr<ReadInstr<bool>>(new LiteralReadInstr<bool>(true)))

TEST(ConcurrentFunctionalTest, SeriesParallelGraph) {
  CharBlockPrinter printer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  x = 'A';
  ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
    x = 'B';
    ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'C';
      } ThisThread::recorder().end_branch();
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'D';
      } ThisThread::recorder().end_branch();
    } ThisThread::recorder().end_branch();
    ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
      x = 'E';
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'F';
      } ThisThread::recorder().end_branch();
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'G';
      } ThisThread::recorder().end_branch();
    } ThisThread::recorder().end_branch();
    ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'H';
      } ThisThread::recorder().end_branch();
      x = 'I';
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'J';
      } ThisThread::recorder().end_branch();
    } ThisThread::recorder().end_branch();
    ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
      x = 'K';
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'L';
      } ThisThread::recorder().end_branch();
      x = 'M';
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'N';
      } ThisThread::recorder().end_branch();
    } ThisThread::recorder().end_branch();
  } ThisThread::recorder().begin_else(); {
    ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'O';
      } ThisThread::recorder().begin_else(); {
        x = 'P';
      } ThisThread::recorder().end_branch();
    } ThisThread::recorder().end_branch();
    x = 'Q';
    ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
      x = 'R';
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'S';
      } ThisThread::recorder().begin_else(); {
        x = 'T';
      } ThisThread::recorder().end_branch();
    } ThisThread::recorder().end_branch();
    ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'U';
      } ThisThread::recorder().begin_else(); {
        x = 'W';
      } ThisThread::recorder().end_branch();
      x = 'V';
    } ThisThread::recorder().end_branch();
    ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
      x = 'X';
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'Y';
      } ThisThread::recorder().end_branch();
      ThisThread::recorder().begin_then(TRUE_READ_INSTR); {
        x = 'Z';
      } ThisThread::recorder().end_branch();
      x = '!';
    } ThisThread::recorder().end_branch();
  } ThisThread::recorder().end_branch();

  printer.print_block_ptr(ThisThread::recorder().most_outer_block_ptr());
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
}

TEST(ConcurrentFunctionalTest, Loop) {
  CharBlockPrinter printer;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  x = 'A';
  char k = 1;
  while (ThisThread::recorder().unwind_loop(x < '?', make_loop_policy<__COUNTER__, 3>())) {
    x = k++;
  }

  printer.print_block_ptr(ThisThread::recorder().most_outer_block_ptr());
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
}

TEST(ConcurrentFunctionalTest, ThreeThreadsReadWriteScalarSharedVar) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  // Declare shared variable initialized by main thread
  SharedVar<char> x;

  // Record first child thread
  Threads::begin_thread();

  x = 'P';

  Threads::end_thread(z3);

  // Record second child thread
  Threads::begin_thread();

  x = 'Q';

  Threads::end_thread(z3);

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
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  a = x;

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::sat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, SatJoinPathsInSingleThreadWithSharedVar) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  if (ThisThread::recorder().begin_then(any<bool>())) {
    x = 'B';
  }
  if (ThisThread::recorder().begin_else()) {
    x = 'C';
  }
  ThisThread::recorder().end_branch();
  a = x;

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(a == 'B');
  std::unique_ptr<ReadInstr<bool>> c1(!(a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c2(a == 'B' && !(a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c3(a == 'C');
  std::unique_ptr<ReadInstr<bool>> c4(!(a == 'B') && a == 'C');
  std::unique_ptr<ReadInstr<bool>> c5(a == 'A');

  Threads::end_main_thread(z3);

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

TEST(ConcurrentFunctionalTest, UnsatJoinPathsInSingleThreadWithSharedVar) {
  Z3 z3;

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

TEST(ConcurrentFunctionalTest, LocalScalarAndLocalArray) {
  Z3 z3;

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

TEST(ConcurrentFunctionalTest, LocalScalarAndSharedArray) {
  Z3 z3;

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

TEST(ConcurrentFunctionalTest, SatJoinPathsInSingleThreadWithNondetermisticConditionAndArraySharedVar) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[3]> x;
  SharedVar<char> y;
  LocalVar<char> a;

  x[2] = 'A';
  if (ThisThread::recorder().begin_then(any<bool>())) {
    x[2] = 'B';
  }
  if (ThisThread::recorder().begin_else()) {
    x[2] = 'C';
  }
  ThisThread::recorder().end_branch();
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

TEST(ConcurrentFunctionalTest, SatJoinPathsInSingleThreadWithDeterminsticConditionAndArraySharedVar) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char[3]> x;
  LocalVar<char> p;
  LocalVar<char> a;

  p = '?';
  x[2] = 'A';
  if (ThisThread::recorder().begin_then(p == '?')) {
    x[2] = 'B'; // choose always 'B' branch
  }
  if (ThisThread::recorder().begin_else()) {
    x[2] = 'C';
  }
  ThisThread::recorder().end_branch();
  a = x[2];

  // Create properties within main thread context
  std::unique_ptr<ReadInstr<bool>> c0(a == 'B');
  std::unique_ptr<ReadInstr<bool>> c1(!(a == 'C'));
  std::unique_ptr<ReadInstr<bool>> c3(!(a == 'B') && a == 'C');

  Threads::end_main_thread(z3);

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

TEST(ConcurrentFunctionalTest, UnsatJoinPathsInSingleThreadWithArraySharedVar) {
  Z3 z3;

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

TEST(ConcurrentFunctionalTest, SatJoinThreads) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  Threads::begin_thread();

  x = 'A';

  Threads::end_thread(z3);

  Threads::error(x == '\0', z3);

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::sat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, UnsatJoinThreads) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;

  Threads::begin_thread();

  x = 'A';

  const std::shared_ptr<SendEvent> send_event_ptr = Threads::end_thread(z3);

  Threads::join(send_event_ptr);
  Threads::error(x == '\0', z3);

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, CopyLocalVar) {
  Z3 z3;

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
  Z3 z3;

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
  Z3 z3;

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
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> var;
  var = any<int>();

  if (ThisThread::recorder().begin_then(0 < var)) {
    Threads::error(var == 0, z3);
  }
  ThisThread::recorder().end_branch();
  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, ConditionalErrorMultipleThreads) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> var;
  var = any<int>();

  Threads::begin_thread();

  if (ThisThread::recorder().begin_then(0 < var)) {
    Threads::error(var == 0, z3);
  }
  ThisThread::recorder().end_branch();

  Threads::end_thread(z3);
  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(ConcurrentFunctionalTest, FalseConditionalError) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> var = 0;

  Threads::begin_thread();
  if (ThisThread::recorder().begin_then(1 == var)) {
    Threads::error(any<int>() == 0, z3);
  }
  ThisThread::recorder().end_branch();
  Threads::end_thread(z3);

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::unsat, z3.solver.check());
}
