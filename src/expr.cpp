// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "expr.h"

namespace se {

const Operator UNARY_BEGIN = NOT;
const Operator UNARY_END = ADD;
const Operator NARY_BEGIN = ADD;
const Operator NARY_END = LSS;

ExprKind ext_expr_kind(unsigned short id) {
  // assumes IDs are sufficiently small
  return static_cast<ExprKind>(EXT_EXPR + id);
}

std::ostream& CastExpr::write(std::ostream& out) const {
  out << LPAR << LPAR;
  out << types[type];
  out << RPAR << LPAR;
  out << expr;
  out << RPAR << RPAR;
  return out;
}

std::ostream& UnaryExpr::write(std::ostream& out) const {
  out << LPAR;
  out << operators[op];
  out << expr;
  out << RPAR;
  return out;
}

std::ostream& TernaryExpr::write(std::ostream& out) const {
  out << LPAR;
  out << cond_expr;
  out << QUERY;
  out << then_expr;
  out << COLON;
  out << else_expr;
  out << RPAR;
  return out;
}

std::ostream& NaryExpr::write(std::ostream& out) const {
  out << LPAR;
  std::list<SharedExpr>::const_iterator iter = exprs.cbegin();
  out << *iter;
  for(iter++; iter != exprs.cend(); iter++) {
    out << operators[op];
    out << *iter;
  }
  out << RPAR;
  return out;
}

}

