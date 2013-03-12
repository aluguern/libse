// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <stdexcept>
#include "expr.h"
#include "core/eval.h"

namespace se {

/*! \file interpreter.h */

template<typename T, Operator op>
class __Operator {
protected:
  static T eval(const T& larg, const T& rarg) {
    return Eval<op>::eval(larg, rarg);
  }

  static T eval(const T& arg) {
    return Eval<op>::eval(arg);
  }
};

template<typename T, Operator op>
class __UnaryOperator : __Operator<T, op> {
public:
  static T build(Visitor<T> * const visitor,
                 const SharedExpr& expr) {

    return __Operator<T, op>::eval(expr->walk(visitor));
  }
};

template<typename T, Operator op>
class __BinaryOperator : __Operator<T, op> {
public:
  static T build(Visitor<T> * const visitor,
                 const SharedExpr& lexpr, const SharedExpr& rexpr) {

    return __Operator<T, op>::eval(lexpr->walk(visitor), rexpr->walk(visitor));
  }
};

template<typename T, Operator op>
class __NaryOperator : __Operator<T, op> {
public:
  static T build(Visitor<T> * const visitor,
                 const T& identity, const std::list<SharedExpr>& exprs) {
    T formula = identity;

    for(SharedExpr expr : exprs) {
      formula = __Operator<T, op>::eval(formula, expr->walk(visitor));
    }

    return formula;
  }
};

/// Unsupported built-in C++ operation or primitive type
typedef std::runtime_error InterpreterException;

/// Symbolic expression interpreter

/// Interprets a symbolic expression through the postorder traversal of its DAG.
/// This way an Interpreter gives semantics to a symbolic expression.
///
/// When the interpreter cannot evaluate an expression, it should throw
/// an InterpreterException with a helpful human-readable error message.
template<typename T>
class Interpreter : public Visitor<T> {
private:
  typedef Visitor<T> * const Self;

public:
  typedef T (*UnaryOperator)(Self, const SharedExpr&);
  typedef T (*BinaryOperator)(Self, const SharedExpr&, const SharedExpr&);
  typedef T (*NaryOperator)(Self, const T& identity, const std::list<SharedExpr>&);

private:
  /* CAUTION: These three arrays depend on the op.h header! */

  // The order of the unary operators must be the same as in the Operator enum
  // data structure. Given such an enum value OP, the appropriate function
  // pointer can be retrieved with m_unary_operators[OP - UNARY_BEGIN]. The
  // size of the array is equal to (UNARY_END - UNARY_BEGIN) + 1.
  static const UnaryOperator m_unary_operators[];

  // The order of the binary operators must be the same as in the Operator enum
  // data structure. Given such an enum value OP, the appropriate function
  // pointer can be retrieved with m_binary_operators[OP - UNARY_END]. The size
  // of the array is equal to (BINARY_END - NARY_BEGIN) + 1.
  static const BinaryOperator m_binary_operators[];

  // An array of associative binary operators. The order of these must be the
  // same as in the Operator enum data structure. Given such an enum value OP,
  // the appropriate function pointer is at m_nary_operators[OP - NARY_BEGIN].
  // The size of the array is equal to (NARY_END - NARY_BEGIN) + 1.
  static const NaryOperator m_nary_operators[];

protected:
  static UnaryOperator find_unary_operator(Operator op) {
    return m_unary_operators[op - UNARY_BEGIN];
  }

  static BinaryOperator find_binary_operator(Operator op) {
    return m_binary_operators[op - NARY_BEGIN];
  }

  static NaryOperator find_nary_operator(Operator op) {
    return m_nary_operators[op - NARY_BEGIN];
  }

public:
  virtual ~Interpreter() {}
};

template<typename T>
const typename Interpreter<T>::
  UnaryOperator Interpreter<T>::m_unary_operators[] =
                        { &__UnaryOperator<T, NOT>::build };

template<typename T>
const typename Interpreter<T>::
  BinaryOperator Interpreter<T>::m_binary_operators[] =
                        { &__BinaryOperator<T, ADD>::build,
                          &__BinaryOperator<T, SUB>::build,
                          &__BinaryOperator<T, LAND>::build,
                          &__BinaryOperator<T, LOR>::build,
                          &__BinaryOperator<T, EQL>::build,
                          &__BinaryOperator<T, LSS>::build };

template<typename T>
const typename Interpreter<T>::
  NaryOperator Interpreter<T>::m_nary_operators[] =
                        { &__NaryOperator<T, ADD>::build,
                          &__NaryOperator<T, LAND>::build,
                          &__NaryOperator<T, LOR>::build };

/// Strongest postcondition predicate transformer

/// Logical statements about an evaluated expression can be proved using the
/// Z3 theorem prover.
class SpInterpreter : public Interpreter<z3::expr> {
public:
  /* Order of fields must match the initialization order! */
  z3::context context;
  z3::solver solver;
  
private:
  z3::expr identities[3];

public:
  SpInterpreter() : context(), solver(context),
            identities { context.int_val(0),
                         context.bool_val(true),
                         context.bool_val(false) } {}

  z3::expr visit(const Expr& other) {
    throw InterpreterException("Expression extensions are unsupported");
  }

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
    const UnaryOperator op = find_unary_operator(expr.op());
    return op(this, expr.operand());
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
      const BinaryOperator op = find_binary_operator(expr.op());
      return op(this, expr.operands().front(), expr.operands().back());
    }
    
    const NaryOperator op = find_nary_operator(expr.op());
    const z3::expr& identity = identities[expr.op() - NARY_BEGIN];
    return op(this, identity, expr.operands());
  }

};

}

#endif /* INTERPRETER_H_ */
