#include "gtest/gtest.h"

#include <sstream>

#include "concurrent.h"

using namespace se;

TEST(ConcurrentFunctionalTest, Counter) {
  constexpr unsigned counter = __COUNTER__;
  EXPECT_EQ(counter + 1, __COUNTER__);
}

TEST(ConcurrentFunctionalTest, LocalArray) {
  // Setup
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
  EXPECT_EQ("(solver\n  (= k!1 ((as const (Array (_ BitVec 64) (_ BitVec 8))) #x00))\n"
            "  (= k!3 (store k!1 #x0000000000000002 #x5a))\n"
            "  (not (= (select k!3 #x0000000000000002) #x5a)))", out.str());
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
  // Setup
  init_recorder();

  CharBlockPrinter printer;
  SharedVar<char> x;

  x = 'A';
  if (recorder_ptr()->begin_then(x == '?')) {
    x = 'B';
  }
  if (recorder_ptr()->begin_else()) {
    x = 'C';
  } recorder_ptr()->end_branch();
  x = 'D';

  printer.print_block_ptr(recorder_ptr()->most_outer_block_ptr());
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
  // Setup
  init_recorder();

  CharBlockPrinter printer;
  SharedVar<char> x;

  x = 'A';
  recorder_ptr()->begin_then(x == '?'); {
  } recorder_ptr()->begin_else(); {
      recorder_ptr()->begin_then(x == '?'); {
        recorder_ptr()->begin_then(x == '?'); {
          x = 'B';
        } recorder_ptr()->begin_else(); {
          x = 'C';
          x = 'D';
        } recorder_ptr()->end_branch();
      } recorder_ptr()->end_branch();
      x = 'E';
  } recorder_ptr()->end_branch();

  printer.print_block_ptr(recorder_ptr()->most_outer_block_ptr());
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

TEST(ConcurrentFunctionalTest, SeriesParallelGraph) {
  // Setup
  init_recorder();

  CharBlockPrinter printer;
  SharedVar<char> x;

  x = 'A';
  recorder_ptr()->begin_then(x == '?'); {
    x = 'B';
    recorder_ptr()->begin_then(x == '?'); {
      recorder_ptr()->begin_then(x == '?'); {
        x = 'C';
      } recorder_ptr()->end_branch();
      recorder_ptr()->begin_then(x == '?'); {
        x = 'D';
      } recorder_ptr()->end_branch();
    } recorder_ptr()->end_branch();
    recorder_ptr()->begin_then(x == '?'); {
      x = 'E';
      recorder_ptr()->begin_then(x == '?'); {
        x = 'F';
      } recorder_ptr()->end_branch();
      recorder_ptr()->begin_then(x == '?'); {
        x = 'G';
      } recorder_ptr()->end_branch();
    } recorder_ptr()->end_branch();
    recorder_ptr()->begin_then(x == '?'); {
      recorder_ptr()->begin_then(x == '?'); {
        x = 'H';
      } recorder_ptr()->end_branch();
      x = 'I';
      recorder_ptr()->begin_then(x == '?'); {
        x = 'J';
      } recorder_ptr()->end_branch();
    } recorder_ptr()->end_branch();
    recorder_ptr()->begin_then(x == '?'); {
      x = 'K';
      recorder_ptr()->begin_then(x == '?'); {
        x = 'L';
      } recorder_ptr()->end_branch();
      x = 'M';
      recorder_ptr()->begin_then(x == '?'); {
        x = 'N';
      } recorder_ptr()->end_branch();
    } recorder_ptr()->end_branch();
  } recorder_ptr()->begin_else(); {
    recorder_ptr()->begin_then(x == '?'); {
      recorder_ptr()->begin_then(x == '?'); {
        x = 'O';
      } recorder_ptr()->begin_else(); {
        x = 'P';
      } recorder_ptr()->end_branch();
    } recorder_ptr()->end_branch();
    x = 'Q';
    recorder_ptr()->begin_then(x == '?'); {
      x = 'R';
      recorder_ptr()->begin_then(x == '?'); {
        x = 'S';
      } recorder_ptr()->begin_else(); {
        x = 'T';
      } recorder_ptr()->end_branch();
    } recorder_ptr()->end_branch();
    recorder_ptr()->begin_then(x == '?'); {
      recorder_ptr()->begin_then(x == '?'); {
        x = 'U';
      } recorder_ptr()->begin_else(); {
        x = 'W';
      } recorder_ptr()->end_branch();
      x = 'V';
    } recorder_ptr()->end_branch();
    recorder_ptr()->begin_then(x == '?'); {
      x = 'X';
      recorder_ptr()->begin_then(x == '?'); {
        x = 'Y';
      } recorder_ptr()->end_branch();
      recorder_ptr()->begin_then(x == '?'); {
        x = 'Z';
      } recorder_ptr()->end_branch();
      x = '!';
    } recorder_ptr()->end_branch();
  } recorder_ptr()->end_branch();

  printer.print_block_ptr(recorder_ptr()->most_outer_block_ptr());
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
  // Setup
  init_recorder();

  CharBlockPrinter printer;
  SharedVar<char> x;

  x = 'A';
  char k = 1;
  while (recorder_ptr()->unwind_loop(x < '?', make_loop_policy<__COUNTER__, 3>())) {
    x = k++;
  }

  printer.print_block_ptr(recorder_ptr()->most_outer_block_ptr());
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

TEST(ConcurrentFunctionalTest, LocalScalarAndArray) {
  // Setup
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
  order_encoder.encode(recorder_ptr()->most_outer_block_ptr(), relation, z3);

  z3.solver.push();

  z3.solver.add(value_encoder.encode(!(a == 'P'), z3));
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(value_encoder.encode(!(a == 'P' || a == '\0'), z3));
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, SatJoinPathsInSingleThreadWithSharedVar) {
  init_recorder();

  Z3 z3;
  MemoryAddrRelation<Event> relation;
  const Z3ValueEncoder value_encoder;
  const Z3OrderEncoder order_encoder;

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  if (recorder_ptr()->begin_then(x == '?')) {
    x = 'B';
  }
  if (recorder_ptr()->begin_else()) {
    x = 'C';
  }
  recorder_ptr()->end_branch();
  a = x;

  recorder_ptr()->encode(value_encoder, z3);
  recorder_ptr()->relate(relation);
  order_encoder.encode(recorder_ptr()->most_outer_block_ptr(), relation, z3);

  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.push();

  z3.solver.add(value_encoder.encode(!(a == 'C'), z3));
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(value_encoder.encode(a == 'B', z3));
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(value_encoder.encode(a == 'B' && !(a == 'C'), z3));
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(value_encoder.encode(!(a == 'B') && a == 'C', z3));
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, UnsatJoinPathsInSingleThreadWithSharedVar) {
  init_recorder();

  Z3 z3;
  MemoryAddrRelation<Event> relation;
  const Z3ValueEncoder value_encoder;
  const Z3OrderEncoder order_encoder;

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  if (recorder_ptr()->begin_then(x == '?')) {
    x = 'B';
  }
  if (recorder_ptr()->begin_else()) {
    x = 'C';
  }
  recorder_ptr()->end_branch();
  a = x;

  recorder_ptr()->encode(value_encoder, z3);
  recorder_ptr()->relate(relation);
  order_encoder.encode(recorder_ptr()->most_outer_block_ptr(), relation, z3);

  z3.solver.push();

  z3.solver.add(value_encoder.encode(!(a == 'B' || a == 'C'), z3));
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(value_encoder.encode(a == 'A', z3));
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(ConcurrentFunctionalTest, SatSingleThreadWithSharedVar) {
  init_recorder();

  Z3 z3;
  MemoryAddrRelation<Event> relation;
  const Z3ValueEncoder value_encoder;
  const Z3OrderEncoder order_encoder;

  SharedVar<char> x;
  LocalVar<char> a;

  x = 'A';
  a = x;

  recorder_ptr()->encode(value_encoder, z3);
  recorder_ptr()->relate(relation);
  order_encoder.encode(recorder_ptr()->most_outer_block_ptr(), relation, z3);

  EXPECT_EQ(z3::sat, z3.solver.check());
}
