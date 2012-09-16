// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <stdexcept>
#include "expr.h"

namespace se {

/*! \file interpreter.h */

/// Calls a member function through a member function pointer

/// Example: CALL_MEMBER_FN(this, foo)(7) calls this->foo(7)
#define CALL_MEMBER_FN(object_ptr,member_fn_ptr) ((object_ptr)->*(member_fn_ptr))

/// Unsupported built-in C++ operation or primitive type
typedef std::runtime_error InterpreterException;

/// \interface Interpreter
/// \brief Symbolic expression evaluator

/// Evaluates a syntactic expression through a DAG postorder traversal.
/// Thus, an Interpreter gives semantics to a symbolic expression.
///
/// When the interpreter cannot evaluate an expression, it should throw
/// an InterpreterException with a helpful human-readable error message.
template<typename T>
class Interpreter : public Visitor<T> {
public:
  virtual ~Interpreter() {}
};

/// Strongest postcondition predicate transformer

/// Logical statements about an evaluated expression can be proved using the
/// Z3 theorem prover.
class SpInterpreter : public Interpreter<z3::expr> {

private:

  // Internal member function pointer for fast dispatching of a unary operator.
  typedef z3::expr (SpInterpreter::*UnaryOperator)(const SharedExpr&);

  // Internal member function pointer for fast dispatching of a binary operator.
  typedef z3::expr (SpInterpreter::*BinaryOperator)(const SharedExpr&, const SharedExpr&);

  // Internal member function pointer for fast dispatching of an associative nary operator.
  typedef z3::expr (SpInterpreter::*NaryOperator)(const std::list<SharedExpr>&);

  /* CAUTION: The size of the following three array *
   * fields depends on the number of operators!     */

  // The order of the unary operators must be the same as in the Operator enum
  // data structure. Given such an enum value OP, its member function pointer
  // can be retrieved with m_unary_operators[OP - UNARY_BEGIN]. The size of the
  // array must be equal to the number of times the UNARY_OP_DEF macro is
  // evaluated inside this class.
  const UnaryOperator m_unary_operators[1 /* see CAUTION note! */];

  // The order of the binary operators must be the same as in the Operator enum
  // data structure. Given such an enum value OP, its member function pointer
  // can be retrieved with m_binary_operators[OP - UNARY_END]. The size of the
  // array must be equal to the number of times the BINARY_OP_DEF macro is
  // evaluated inside this class.
  const BinaryOperator m_binary_operators[5 /* see CAUTION note! */];

  // An array of associative binary operators. The order of these must be the
  // same as in the Operator enum data structure. Given such an enum value OP,
  // its member function pointer can be retrieved with
  // m_nary_operators[OP - NARY_BEGIN]. The size of the array must be equal to
  // the number of member function definitions of type NaryOperator.
  const NaryOperator m_nary_operators[3 /* see CAUTION note! */];


/// Defines a member function of type UnaryOperator

/// The number of times this macro is called inside this class definition must
/// be equal to the static size of the internal m_unary_operators array.
#define UNARY_OP_DEF(op, op_name)\
  z3::expr op_name(const SharedExpr& expr) {\
    return op expr->walk(this);\
  }

UNARY_OP_DEF(!, NOT_1)

/// Defines a member function of type BinaryOperator

/// The number of times this macro is called inside this class definition must
/// be equal to the static size of the internal m_binary_operators array.
#define BINARY_OP_DEF(op, op_name)\
  z3::expr op_name(const SharedExpr& x_expr, const SharedExpr& y_expr) {\
    return x_expr->walk(this) op y_expr->walk(this);\
  }

BINARY_OP_DEF(+, ADD_2)
BINARY_OP_DEF(&&, LAND_2)
BINARY_OP_DEF(||, LOR_2)
BINARY_OP_DEF(==, EQL_2)
BINARY_OP_DEF(<, LSS_2)


  // ADD_N defines a member function of type NaryOperator that sums up an
  // arbitrary number of integer expressions. There must be at least two
  // summands.
  z3::expr ADD_N(const std::list<SharedExpr>& exprs) {
    // Zero is the additive identity element
    z3::expr z3_expr = context.int_val(0);

    //TODO: Use Z3_mk_add
    for(SharedExpr expr : exprs) {
      z3_expr = z3_expr + expr->walk(this);
    }

    return z3_expr;
  }

  z3::expr LAND_N(const std::list<SharedExpr>& exprs) {
    // True is the identity element for logical AND
    z3::expr z3_expr = context.bool_val(true);

    //TODO: Use Z3_mk_and
    for(SharedExpr expr : exprs) {
      z3_expr = z3_expr && expr->walk(this);
    }

    return z3_expr;
  }


  z3::expr LOR_N(const std::list<SharedExpr>& exprs) {
    // False is the identity element for logical OR
    z3::expr z3_expr = context.bool_val(false);

    //TODO: Use Z3_mk_or
    for(SharedExpr expr : exprs) {
      z3_expr = z3_expr || expr->walk(this);
    }

    return z3_expr;
  }

public:

  // C++ API to Z3 Theorem Prover
  z3::context context;
  z3::solver solver;
  
  SpInterpreter() : context(), solver(context),
    m_unary_operators{&SpInterpreter::NOT_1},
    m_binary_operators{&SpInterpreter::ADD_2, &SpInterpreter::LAND_2,
                     &SpInterpreter::LOR_2, &SpInterpreter::EQL_2,
                     &SpInterpreter::LSS_2},
    m_nary_operators{&SpInterpreter::ADD_N, &SpInterpreter::LAND_N, &SpInterpreter::LOR_N} {}
 
  z3::expr visit(const AnyExpr<bool>& expr) {
    return context.bool_const(expr.identifier().c_str());
  }

  z3::expr visit(const ValueExpr<bool>& expr) {
    return context.bool_val(expr.value());
  }

  z3::expr visit(const AnyExpr<char>& expr) {
    throw InterpreterException("Char variables are currently unsupported.");
  }

  z3::expr visit(const ValueExpr<char>& expr) {
    throw InterpreterException("Char values are currently unsupported.");
  }

  z3::expr visit(const AnyExpr<short int>& expr) {
    throw InterpreterException("Short int variables are currently unsupported.");
  }

  z3::expr visit(const ValueExpr<short int>& expr) {
    throw InterpreterException("Short int values are currently unsupported.");
  }

  z3::expr visit(const AnyExpr<int>& expr) {
    return context.int_const(expr.identifier().c_str());
  }

  z3::expr visit(const ValueExpr<int>& expr) {
    return context.int_val(expr.value());
  }

  z3::expr visit(const CastExpr& expr) {
    throw InterpreterException("Casts are currently unsupported.");
  }

  z3::expr visit(const UnaryExpr& expr) {
    const UnaryOperator op = m_unary_operators[expr.op() - UNARY_BEGIN];
    return CALL_MEMBER_FN(this, op)(expr.operand());
  }

  z3::expr visit(const IfThenElseExpr& expr) {
    z3::expr cond_expr = expr.cond_expr()->walk(this);
    z3::expr then_expr = expr.then_expr()->walk(this);
    z3::expr else_expr = expr.else_expr()->walk(this);

    Z3_ast r = Z3_mk_ite(cond_expr.ctx(), cond_expr, then_expr, else_expr);
    cond_expr.check_error();

    return z3::expr(cond_expr.ctx(), r);
  }

  z3::expr visit(const NaryExpr& expr) {
    const size_t operands = expr.operands().size();
    if(operands < 1) {
      throw InterpreterException("NaryExpr must have at least one operand.");
    }

    if(operands == 1) {
      return expr.operands().front()->walk(this);
    }

    if(operands == 2) {
      const BinaryOperator op = m_binary_operators[expr.op() - UNARY_END];
      return CALL_MEMBER_FN(this, op)(expr.operands().front(), expr.operands().back());
    }
    
    const NaryOperator op = m_nary_operators[expr.op() - NARY_BEGIN];
    return CALL_MEMBER_FN(this, op)(expr.operands());
  }

};

}

#endif /* INTERPRETER_H_ */
