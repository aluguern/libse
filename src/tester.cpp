// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <iostream>
#include "tester.h"

namespace se {

bool tester::If::test_then() {
  pop = true;
  solver.push();
  solver.add(se::If::cond.get_expr()->walk(sp_interpreter_ptr));
  if (solver.check() == z3::sat) {
    generator_ptr->generate(solver.get_model());
  }
  return solver.check() == z3::sat;
}

bool tester::If::test_else() {
  pop = true;
  solver.push();
  const Value<bool>& not_cond = !se::If::cond;
  solver.add(not_cond.get_expr()->walk(sp_interpreter_ptr));
  if (solver.check() == z3::sat) {
    generator_ptr->generate(solver.get_model());
  }
  return solver.check() == z3::sat;
}

bool tester::If::begin_then() {
  if (se::If::begin_then()) {
    return test_then();
  } else {
    return false;
  }
}

bool tester::If::begin_else() {
  if (pop) { solver.pop(); pop = false; }

  if (se::If::begin_else()) {
    return test_else();
  } else {
    return false;
  }
}

void tester::If::end() {
  if (pop) { solver.pop(); pop = false; }
  if (!is_if_then_else()) { test_else(); }
  se::If::end();
}

bool tester::Loop::unwind(const Value<bool>& cond) {
  if (!init) { solver.pop(); }
  init = false;

  solver.push();
  const Value<bool>& not_cond = !cond;
  solver.add(not_cond.get_expr()->walk(sp_interpreter_ptr));
  if (solver.check() == z3::sat) {
    generator_ptr->generate(solver.get_model());
  }

  solver.pop();
  solver.push();
  solver.add(cond.get_expr()->walk(sp_interpreter_ptr));
  if (solver.check() == z3::unsat) {
    return false;
  }

  generator_ptr->generate(solver.get_model());

  return se::Loop::unwind(cond);
}

}
