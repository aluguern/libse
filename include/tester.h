// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef TESTER_H_
#define TESTER_H_

#include <z3++.h>
#include <ostream>

#include "sequential-se.h"
#include "interpreter.h"

namespace se {

// Classes in the tester namespace call a decision procedure on each control
// point where a symbolic Boolean condition determines the control-flow of the
// program under test. If such a symbolic condition is satisfiable, then a
// satisfying solution serves as a test case which can contribute towards the
// branch coverage of the program under test.
//
// Since the tester namespace implements several custom versions of the libse
// symbolic execution API, these custom implementations should only be used
// with the qualified namespace to avoid confusion with the mainstream library.
namespace tester {

// Interface to generate a test case from a Z3 model.
class TestGenerator {
private:
  TestGenerator(const TestGenerator&);
  TestGenerator& operator=(const TestGenerator&);

protected:
  TestGenerator() {}

public:
  virtual void generate(const z3::model&) = 0;
  virtual ~TestGenerator() {}
};

typedef std::shared_ptr<TestGenerator> TestGeneratorPtr;

// Writes a Z3 model to an output stream. It is the responsibility of the
// caller to manage the output stream.
class TestCaseStream : public TestGenerator {
private:
  std::ostream* ostream_ptr;

public:
  TestCaseStream(std::ostream* ostream_ptr) : ostream_ptr(ostream_ptr) {}
  void generate(const z3::model& model) {
    (*ostream_ptr) << "[" << model << "]" << std::endl;
  }
  ~TestCaseStream() {}
};
    
// tester::If calls a decision procedure twice if the branch condition is
// symbolic: one to find a satisfying assignment for the "then" branch and
// another for the "else" branch. Since these conditions are disjoint, at
// least one solution can always be found.
class If : public se::If {
private:
  bool pop;
  SpInterpreter* const sp_interpreter_ptr;
  z3::solver solver;
  TestGeneratorPtr generator_ptr;

  bool test_then();
  bool test_else();

public:
  If(const Value<bool> cond, SpInterpreter& sp_interpreter,
     const TestGeneratorPtr& generator_ptr) :
    se::If(cond), pop(false), sp_interpreter_ptr(&sp_interpreter),
    solver(sp_interpreter.solver), generator_ptr(generator_ptr) {}

  bool begin_then();
  bool begin_else();
  void end();
};

// If the loop unwinding bound of tester::Loop has not been exceeded and the
// loop condition is symbolic, then a decision procedure is called in an
// attempt to obtain two satisfying assignments: one to exit the loop and
// another to continue the loop.
class Loop : public se::Loop {
private:
  bool init;
  SpInterpreter* const sp_interpreter_ptr;
  z3::solver solver;
  TestGeneratorPtr generator_ptr;

public:
  Loop(const unsigned int k, SpInterpreter& sp_interpreter,
       const TestGeneratorPtr& generator_ptr) :
    se::Loop(k), init(true), sp_interpreter_ptr(&sp_interpreter),
    solver(sp_interpreter.solver), generator_ptr(generator_ptr) {}

  bool unwind(const Value<bool>&);
};

}
}

#endif /* TESTER_H_ */
