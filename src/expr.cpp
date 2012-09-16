// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "expr.h"

namespace se {

const Operator UNARY_BEGIN = NOT;
const Operator UNARY_END = ADD;
const Operator NARY_BEGIN = ADD;
const Operator NARY_END = LSS;

std::ostream& CastExpr::write(std::ostream& out) const {
  out << LPAR << LPAR;
  out << types[m_type];
  out << RPAR << LPAR;
  out << m_operand;
  out << RPAR << RPAR;
  return out;
}

std::ostream& UnaryExpr::write(std::ostream& out) const {
  out << LPAR;
  out << operators[m_op];
  out << m_operand;
  out << RPAR;
  return out;
}

std::ostream& IfThenElseExpr::write(std::ostream& out) const {
  out << LPAR;
  out << m_cond_expr;
  out << QUERY;
  out << m_then_expr;
  out << COLON;
  out << m_else_expr;
  out << RPAR;
  return out;
}

std::ostream& NaryExpr::write(std::ostream& out) const {
  out << LPAR;
  std::list<SharedExpr>::const_iterator iter = m_operands.cbegin();

  if(iter == m_operands.cend()) {
    out << RPAR;
    return out;
  }

  out << *iter;
  for(iter++; iter != m_operands.cend(); iter++) {
    out << operators[m_op];
    out << *iter;
  }
  out << RPAR;
  return out;
}

}

