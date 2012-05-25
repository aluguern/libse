#ifndef TRACER_H_
#define TRACER_H_

#include "expr.h"
#include <list>

namespace sp {

class Tracer {
private:
  std::list<SharedExpr> path_constraints;

public:
  void reset();
  void add_path_constraint(const SharedExpr& expr);

  std::ostream& write_path_constraints(std::ostream& out) const;
};

}

#endif /* TRACER_H_ */
