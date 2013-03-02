// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef INSTR_H_
#define INSTR_H_

#include "eval.h"
#include "value.h"

namespace se {

// PARTIAL_EXPR is a strictly internal macro that instantiates a new nary
// expression that has got only one operand. It is the responsibility of the
// caller to ensure that another NaryExpr modifier function is invoked.
#define PARTIAL_EXPR(op, expr) \
  SharedExpr(new NaryExpr(op, OperatorInfo<op>::attr, (expr)))

// BINARY_EXPR is a strictly internal macro that instantiates a new binary
// expression for the given binary operator enum value and operands.
#define BINARY_EXPR(op, x_expr, y_expr) \
  SharedExpr(new NaryExpr(op, OperatorInfo<op>::attr, (x_expr), (y_expr)))

/// Evaluate and simplify symbolic expressions
template<Operator op>
class Instr {
public:
  template<typename T, typename U, typename V>
  static void exec(const T& larg, const U& rarg, V& result) {
    if(larg.is_symbolic() || rarg.is_symbolic()) {
      result.set_expr(BINARY_EXPR(op, larg.expr(), rarg.expr()));
    }
  }

  template<typename T, typename U>
  static void exec(const T& larg, int rarg, U& result) {
    if(larg.is_symbolic()) {
      SharedExpr raw_expr = larg.AbstractValue::expr();

      if(OperatorInfo<op>::is_commutative_monoid()) {
        bool create_partial_nary_expr = false;
        auto kind = raw_expr->kind();
        if(kind == ANY_EXPR || kind == VALUE_EXPR) {
          create_partial_nary_expr = true;
        } else if(kind == NARY_EXPR) {
          auto nary_expr = std::dynamic_pointer_cast<NaryExpr>(raw_expr);
          if(nary_expr->attr() == OperatorInfo<op>::attr) {
            if(nary_expr->is_partial()) {
              const auto new_aggregate = Eval<op>::eval(larg.aggregate(), rarg);
              result.set_aggregate(new_aggregate);
              result.set_expr(raw_expr);
              return;
            } else {
              create_partial_nary_expr = true;
            }
          }
        }
        if(create_partial_nary_expr) {
          /* larg.data() must act as the identity element of op. */
          // TODO: Support pointer modifications, i.e. (int* i) + N
          result.set_aggregate(rarg);
          result.set_expr(PARTIAL_EXPR(op, raw_expr));
          return;
        }
      }

      result.set_expr(BINARY_EXPR(op, larg.expr(), Value<int>(rarg).expr()));
    }
  }

  template<typename T, typename U>
  static void exec(int larg, const T& rarg, U& result) {
    if(rarg.is_symbolic()) {
      result.set_expr(BINARY_EXPR(op, Value<int>(larg).expr(), rarg.expr()));
    }
  }

  template<typename T, typename U> 
  static void exec(const T& arg, U& result) {
    if(arg.is_symbolic()) {
      result.set_expr(SharedExpr(new UnaryExpr(op, arg.expr())));
    }
  }
};

}

#endif /* INSTR_H_ */
