#include <sstream>
#include "gtest/gtest.h"
#include "sequential-se.h"
#include "tester.h"

TEST(TesterFunctionalTest, IfThen) {
  se::SpInterpreter sp_interpreter;
  std::stringstream out;
  se::tester::TestGeneratorPtr generator_ptr(new se::tester::TestCaseStream(&out));

  int MAX = 5;
  se::Int i = se::any<int>("I");

  se::tester::If branch(i == 3, sp_interpreter, generator_ptr);
  branch.track(i);
  if (branch.begin_then()) { i = i + MAX; }
  branch.end();

  EXPECT_EQ("[(define-fun I () Int\n  3)]\n[(define-fun I () Int\n  0)]\n", out.str());
}

TEST(TesterFunctionalTest, IfThenElse) {
  se::SpInterpreter sp_interpreter;
  std::stringstream out;
  se::tester::TestGeneratorPtr generator_ptr(new se::tester::TestCaseStream(&out));

  int MAX = 3;
  se::Int j = se::any<int>("J");

  se::tester::If branch(j < 0, sp_interpreter, generator_ptr);
  branch.track(j);
  if (branch.begin_then()) { j = j + MAX; }
  if (branch.begin_else()) { j = j + se::any<int>("A");}
  branch.end();

  EXPECT_EQ("[(define-fun J () Int\n  (- 1))]\n[(define-fun J () Int\n  0)]\n", out.str());
}

TEST(TesterFunctionalTest, SeveralIfThenElseWithMultipleVars) {
  se::SpInterpreter sp_interpreter;
  std::stringstream out;
  se::tester::TestGeneratorPtr generator_ptr(new se::tester::TestCaseStream(&out));

  se::Int x = se::any<int>("X");
  se::Int y = se::any<int>("Y");

  se::tester::If branch_0(x < 0, sp_interpreter, generator_ptr);
  branch_0.track(x);
  branch_0.track(y);
  if (branch_0.begin_then()) { y = 5; }
  if (branch_0.begin_else()) { x = x + 4;}
  branch_0.end();

  const std::string x_test_case = "[(define-fun X () Int\n  (- 1))]\n"
                                  "[(define-fun X () Int\n  0)]\n";
  EXPECT_EQ(x_test_case, out.str());

  se::tester::If branch_1(y == 5, sp_interpreter, generator_ptr);
  branch_1.track(x);
  branch_1.track(y);
  if (branch_1.begin_then()) { x = y + 7; }
  branch_1.end();

  const std::string y_test_case = "[(define-fun X () Int\n  (- 1))]\n"
                                  "[(define-fun X () Int\n  0)\n(define-fun Y () Int\n  1)]\n";
  EXPECT_EQ(x_test_case + y_test_case, out.str());
}

TEST(TesterFunctionalTest, Loop) {
  se::SpInterpreter sp_interpreter;
  std::stringstream out;
  se::tester::TestGeneratorPtr generator_ptr(new se::tester::TestCaseStream(&out));

  se::Int p = se::any<int>("P");

  se::tester::Loop loop(2, sp_interpreter, generator_ptr);
  loop.track(p);
  while(loop.unwind(p < 5)) {
    p = p + 1;
  }

  EXPECT_EQ("[(define-fun P () Int\n  5)]\n"
            "[(define-fun P () Int\n  0)]\n"
            "[(define-fun P () Int\n  4)]\n"
            "[(define-fun P () Int\n  0)]\n"
            "[(define-fun P () Int\n  3)]\n"
            "[(define-fun P () Int\n  0)]\n", out.str());
}

TEST(TesterFunctionalTest, NestedIfThenElse) {
  se::SpInterpreter sp_interpreter;
  std::stringstream out;
  se::tester::TestGeneratorPtr generator_ptr(new se::tester::TestCaseStream(&out));

  se::Int p = se::any<int>("P");
  se::Int q = se::any<int>("Q");
  se::Bool r = true;

  se::tester::If outer_branch(p < 5, sp_interpreter, generator_ptr);
  outer_branch.track(r);
  if(outer_branch.begin_then()) {
    se::tester::If inner_branch(q < 3, sp_interpreter, generator_ptr);
    inner_branch.track(r);
    if (inner_branch.begin_then()) { r = false; }
    inner_branch.end();
  }
  outer_branch.end();

  EXPECT_EQ("[(define-fun P () Int\n  0)]\n"
            "[(define-fun Q () Int\n  0)\n(define-fun P () Int\n  0)]\n"
            "[(define-fun Q () Int\n  3)\n(define-fun P () Int\n  0)]\n", out.str());
}
