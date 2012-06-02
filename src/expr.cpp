#include "expr.h"

namespace se {

const Operator UNARY_BEGIN = NOT;
const Operator UNARY_END = ADD;
const Operator BINARY_BEGIN = ADD;
const Operator BINARY_END = LSS;

std::ostream& CastExpr::write(std::ostream& out) const {
  out << LPAR << LPAR;
  out << types[type];
  out << RPAR << LPAR;
  expr->write(out);
  out << RPAR << RPAR;
  return out;
}

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

}

