#include "expr.h"

namespace sp {

std::ostream& UnaryExpr::write(std::ostream& out) const {
  out << LPAR;
  out << operators[op];
  expr->write(out);
  out << RPAR;
  return out;
}

std::ostream& BinaryExpr::write(std::ostream& out) const {
  out << LPAR;
  x_expr->write(out);
  out << operators[op];
  y_expr->write(out);
  out << RPAR;
  return out;
}

std::ostream& TernaryExpr::write(std::ostream& out) const {
  out << LPAR;
  cond_expr->write(out);
  out << QUERY;
  then_expr->write(out);
  out << COLON;
  else_expr->write(out);
  out << RPAR;
  return out;
}

std::ostream& CastExpr::write(std::ostream& out) const {
  out << LPAR << LPAR;
  out << types[type];
  out << RPAR << LPAR;
  expr->write(out);
  out << RPAR << RPAR;
  return out;
}

}

