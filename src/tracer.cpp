#include "tracer.h"

namespace sp {

void Tracer::reset() {
  path_constraints.clear();
}

void Tracer::add_path_constraint(const SharedExpr& expr) {
  path_constraints.push_back(expr);
}

std::ostream& Tracer::write_path_constraints(std::ostream& out) const {
  for (auto it = path_constraints.begin(); it != path_constraints.end(); it++) {
    (*it)->write(out);
    out << std::endl;
  }

  return out;
}

}

