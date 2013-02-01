// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "expr.h"

namespace se {

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

std::ostream& ArrayExpr::write(std::ostream& out) const {
  out << LSQPAR << m_identifier << RSQPAR;
  return out;
}

std::ostream& SelectExpr::write(std::ostream& out) const {
  out << "Select" << LPAR;
  out << m_array_expr;
  out << COMMA << SPACE;
  out << m_index_expr << RPAR;
  return out;
}

std::ostream& StoreExpr::write(std::ostream& out) const {
  out << "Store" << LPAR;
  out << m_array_expr;
  out << COMMA << SPACE;
  out << m_index_expr << COMMA << SPACE;
  out << m_elem_expr << RPAR;
  return out;
}


}

