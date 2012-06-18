// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "tracer.h"

namespace se {

void Tracer::reset() {
  path_constraints.clear();
}

void Tracer::add_path_constraint(const SharedExpr& expr) {
  path_constraints.push_back(expr);
}

std::ostream& Tracer::write_path_constraints(std::ostream& out) const {
  for (auto it = path_constraints.begin(); it != path_constraints.end(); it++) {
    out << *it;
    out << std::endl;
  }

  return out;
}

}

