// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <iostream>
#include "tester.h"

namespace se {

bool tester::If::test_then() {
  m_pop = true;
  m_solver.push();
  m_solver.add(se::If::cond.expr()->walk(m_sp_interpreter_ptr));
  if (m_solver.check() == z3::sat) {
    m_generator_ptr->generate(m_solver.get_model());
  }
  return m_solver.check() == z3::sat;
}

bool tester::If::test_else() {
  m_pop = true;
  m_solver.push();
  const Value<bool>& not_cond = !se::If::cond;
  m_solver.add(not_cond.expr()->walk(m_sp_interpreter_ptr));
  if (m_solver.check() == z3::sat) {
    m_generator_ptr->generate(m_solver.get_model());
  }
  return m_solver.check() == z3::sat;
}

bool tester::If::begin_then() {
  if (se::If::begin_then()) {
    return test_then();
  } else {
    return false;
  }
}

bool tester::If::begin_else() {
  if (m_pop) { m_solver.pop(); m_pop = false; }

  if (se::If::begin_else()) {
    return test_else();
  } else {
    return false;
  }
}

void tester::If::end() {
  if (m_pop) { m_solver.pop(); m_pop = false; }
  if (!is_if_then_else()) { test_else(); }
  se::If::end();
}

bool tester::Loop::unwind(const Value<bool>& cond) {
  if (!m_init) { m_solver.pop(); }
  m_init = false;

  m_solver.push();
  const Value<bool>& not_cond = !cond;
  m_solver.add(not_cond.expr()->walk(m_sp_interpreter_ptr));
  if (m_solver.check() == z3::sat) {
    m_generator_ptr->generate(m_solver.get_model());
  }

  m_solver.pop();
  m_solver.push();
  m_solver.add(cond.expr()->walk(m_sp_interpreter_ptr));
  if (m_solver.check() == z3::unsat) {
    return false;
  }

  m_generator_ptr->generate(m_solver.get_model());

  return se::Loop::unwind(cond);
}

}
