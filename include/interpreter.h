// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <stdexcept>
#include "expr.h"

namespace se {

// Example: CALL_MEMBER_FN(this, foo)(7) calls this->foo(7).
#define CALL_MEMBER_FN(object_ptr,member_fn_ptr) ((object_ptr)->*(member_fn_ptr))

// InterpreterException is thrown when the interpreter does not support
// all the built-in C++ operations or primitive types.
typedef std::runtime_error InterpreterException;

// Interpreter is an interface that evaluates a syntactic expression by
// traversing the DAG in postorder. This traversal can be the basis for
// the implementation of abstract interpretation algorithms which operate
// over lattice elements of type T.
//
// When the interpreter cannot evaluate an expression, it should throw
// an InterpreterException with a helpful human-readable error message.
template<typename T>
class Interpreter : public Visitor<T> {
public:
  virtual ~Interpreter() {}
};

// SpInterpreter is an abstract interpreter that implements the strongest
// postcondition transformer semantics. Logical statements about an
// evaluated expression can be proved using the Z3 theorem prover.
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

  // unary_operators is an array of unary operators. The order of these unary
  // operators must be the same as in the Operator enum data structure. Given
  // such an enum value OP, its member function pointer can be retrieved with
  // unary_operators[OP - UNARY_BEGIN]. The size of the array must be equal to
  // the number of times the UNARY_OP_DEF macro is evaluated inside this class.
  const UnaryOperator unary_operators[1 /* CAUTION! */];

  // binary_operators is an array of binary operators. The order of these
  // binary operators must be the same as in the Operator enum data structure.
  // Given such an enum value OP, its member function pointer can be retrieved
  // with the index OP - UNARY_END. The size of the array must be equal to the
  // number of times the BINARY_OP_DEF macro is evaluated inside this class.
  const BinaryOperator binary_operators[5 /* CAUTION! */];

  // nary_operators is an array of associative binary operators. The order of
  // these must be the same as in the Operator enum data structure. Given such
  // an enum value OP, its member function pointer can be retrieved with the
  // index OP - NARY_BEGIN. The size of the array must be equal to the number
  // of member function definitions of type NaryOperator.
  const NaryOperator nary_operators[3 /* CAUTION! */];


// UNARY_OP_DEF defines a member function of type UnaryOperator. The number of
// times this macro is called inside this class definition must be equal to the
// size of the unary_operators array.
#define UNARY_OP_DEF(op, op_name)\
  z3::expr op_name(const SharedExpr& expr) {\
    return op expr->walk(this);\
  }

UNARY_OP_DEF(!, NOT_1)


// BINARY_OP_DEF defines a member function of type BinaryOperator. The number
// of times this macro is called inside this class definition must be equal
// to the size of the binary_operators array.
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

  // C++ API to Z3 Theorem Prover context
  z3::context context;

  SpInterpreter() : context(),
    unary_operators{&SpInterpreter::NOT_1},
    binary_operators{&SpInterpreter::ADD_2, &SpInterpreter::LAND_2,
                     &SpInterpreter::LOR_2, &SpInterpreter::EQL_2,
                     &SpInterpreter::LSS_2},
    nary_operators{&SpInterpreter::ADD_N, &SpInterpreter::LAND_N, &SpInterpreter::LOR_N} {}
 
  z3::expr visit(const AnyExpr<bool>& expr) {
    return context.bool_const(expr.get_name().c_str());
  }

  z3::expr visit(const ValueExpr<bool>& expr) {
    return context.bool_val(expr.get_value());
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
    return context.int_const(expr.get_name().c_str());
  }

  z3::expr visit(const ValueExpr<int>& expr) {
    return context.int_val(expr.get_value());
  }

  z3::expr visit(const CastExpr& expr) {
    throw InterpreterException("Casts are currently unsupported.");
  }

  z3::expr visit(const UnaryExpr& expr) {
    const UnaryOperator op = unary_operators[expr.get_op() - UNARY_BEGIN];
    return CALL_MEMBER_FN(this, op)(expr.get_expr());
  }

  z3::expr visit(const TernaryExpr& expr) {
    z3::expr cond_expr = expr.get_cond_expr()->walk(this);
    z3::expr then_expr = expr.get_then_expr()->walk(this);
    z3::expr else_expr = expr.get_else_expr()->walk(this);

    Z3_ast r = Z3_mk_ite(cond_expr.ctx(), cond_expr, then_expr, else_expr);
    cond_expr.check_error();

    return z3::expr(cond_expr.ctx(), r);
  }

  z3::expr visit(const NaryExpr& expr) {
    const size_t operands = expr.get_exprs().size();
    if(operands < 2) {
      throw InterpreterException("NaryExpr must have at least two operands.");
    }

    if(operands == 2) {
      const BinaryOperator op = binary_operators[expr.get_op() - UNARY_END];
      return CALL_MEMBER_FN(this, op)(expr.get_exprs().front(), expr.get_exprs().back());
    }
    
    const NaryOperator op = nary_operators[expr.get_op() - NARY_BEGIN];
    return CALL_MEMBER_FN(this, op)(expr.get_exprs());
  }

};

}

#endif /* INTERPRETER_H_ */
