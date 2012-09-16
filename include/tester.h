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

/// Test generator namespace

/// Classes in the tester namespace call a decision procedure on each control
/// point where a symbolic Boolean condition determines the control-flow of the
/// program under test. If such a symbolic condition is satisfiable, then a
/// satisfying solution can serve as a test case which can contribute towards
/// the branch coverage of the program under test.
///
/// A TestGenerator combines the strength of multi-path symbolic execution
/// and incremental SAT/SMT solving. The solving is further optimized by
/// simplifying symbolic expressions through constant propagation and dynamic
/// program slicing of the program under test.
///
/// Since the tester namespace implements several custom versions of the libse
/// symbolic execution API, these custom implementations should only be used
/// with the qualified namespace to avoid confusion with the main library.
namespace tester {

/// \interface TestGenerator
/// \brief Test case generator
class TestGenerator {
private:
  TestGenerator(const TestGenerator&);
  TestGenerator& operator=(const TestGenerator&);

protected:
  TestGenerator() {}

public:
  /// Generates a test case from a Z3 model
  virtual void generate(const z3::model&) = 0;
  virtual ~TestGenerator() {}
};

typedef std::shared_ptr<TestGenerator> TestGeneratorPtr;

/// Z3 model serializer

/// It is the responsibility of the caller to manage the output stream.
class TestCaseStream : public TestGenerator {
private:
  std::ostream* m_ostream_ptr;

public:
  TestCaseStream(std::ostream* m_ostream_ptr) : m_ostream_ptr(m_ostream_ptr) {}

  /// Writes a Z3 model to an output stream
  void generate(const z3::model& model) {
    (*m_ostream_ptr) << "[" << model << "]" << std::endl;
  }
  ~TestCaseStream() {}
};

/// Test generator for an if-then-else statement

/// Calls a decision procedure twice if the branch condition is symbolic:
/// one to find a satisfying assignment for the "then" branch and one for the
/// "else" branch. Since these conditions are disjoint, at least one solution
/// can always be found.
class If : public se::If {
private:
  bool m_pop;
  SpInterpreter* const m_sp_interpreter_ptr;
  z3::solver m_solver;
  TestGeneratorPtr m_generator_ptr;

  bool test_then();
  bool test_else();

public:
  If(const Value<bool> cond, SpInterpreter& sp_interpreter,
     const TestGeneratorPtr& m_generator_ptr) :
    se::If(cond), m_pop(false), m_sp_interpreter_ptr(&sp_interpreter),
    m_solver(sp_interpreter.solver), m_generator_ptr(m_generator_ptr) {}

  bool begin_then();
  bool begin_else();
  void end();
};

/// Test generator for a loop statement

/// If the loop unwinding bound of tester::Loop has not been exceeded and the
/// loop condition is symbolic, then a decision procedure is called in an
/// attempt to obtain two satisfying assignments: one to exit the loop and
/// another to continue the loop.
class Loop : public se::Loop {
private:
  bool m_init;
  SpInterpreter* const m_sp_interpreter_ptr;
  z3::solver m_solver;
  TestGeneratorPtr m_generator_ptr;

public:
  Loop(const unsigned int k, SpInterpreter& sp_interpreter,
       const TestGeneratorPtr& m_generator_ptr) :
    se::Loop(k), m_init(true), m_sp_interpreter_ptr(&sp_interpreter),
    m_solver(sp_interpreter.solver), m_generator_ptr(m_generator_ptr) {}

  bool unwind(const Value<bool>&);
};

}
}

#endif /* TESTER_H_ */
