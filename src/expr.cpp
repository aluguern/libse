#include "expr.h"

std::ostream& UnaryExpr::write(std::ostream& out) const {
  out << LPAR;
  out << operators[op];
  expr->write(out);
  out << RPAR;
  return out;
}

std::ostream& BinaryExpr::write(std::ostream& out) const {
  out << LPAR;
  x->write(out);
  out << operators[op];
  y->write(out);
  out << RPAR;
  return out;
}

std::ostream& TernaryExpr::write(std::ostream& out) const {
  out << LPAR;
  x->write(out);
  out << QUERY;
  y->write(out);
  out << COLON;
  z->write(out);
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

