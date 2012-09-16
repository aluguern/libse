// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef SE_H_
#define SE_H_

#include "value.h"
#include "var.h"
#include "if.h"
#include "loop.h"

/// \mainpage
/// An extensible symbolic execution library for C++ code
///
/// The library caters for both single-path and multi-path symbolic execution,
/// i.e. DART-style, KLEE-style and CBMC-style symbolic execution.
///
/// Symbolic variables are encapsulated as se::Var whose value can be
/// inspected through se::Value.
///
/// Multi-path symbolic execution requires the source annotation of the program
/// under test with se::Loop and se::If.  
///
/// A prototype of a new automated test generator can be found in se::tester.

// Internal function to guide the type inference of template functions that
// overload built-in operators such as addition, multiplication etc.
template<typename T>
static inline const se::Value<T>& __filter(const se::Var<T>& var) {
  return var.value();
}

// Internal function to guide the type inference of template functions that
// overload built-in operators such as addition, multiplication etc.
template<typename T>
static inline const se::Value<T>& __filter(const se::Value<T>& value) {
  return value;
}

// PARTIAL_EXPR is a strictly internal macro that instantiates a new nary
// expression that has got only one operand. It is the responsibility of the
// caller to ensure that another NaryExpr modifier is invoked.
#define PARTIAL_EXPR(opname, expr) \
  se::SharedExpr(new se::NaryExpr(se::opname,\
    se::OperatorInfo<se::opname>::attr, (expr)))

// BINARY_EXPR is a strictly internal macro that instantiates a new binary
// expression for the given binary operator enum value and operands.
#define BINARY_EXPR(opname, x_expr, y_expr) \
  se::SharedExpr(new se::NaryExpr(se::opname,\
    se::OperatorInfo<se::opname>::attr, (x_expr), (y_expr)))

// OVERLOAD_BINARY_OPERATOR(op, opname) is a macro that uses templates and the
// statically overloaded and inlined __filter functions to implement a custom
// C++ binary operator "op" whose reflection identifier is "opname". These
// binary operators are assumed to be associative and commutative. If this is
// not the case (e.g. IEEE 754 floating-point addition), function template
// specializations should be used to deal with these cases separately.
//
// Note that none of the template types can ever be native or smart pointers.
#define OVERLOAD_BINARY_OPERATOR(op, opname) \
  template<typename X, typename Y>\
  const auto operator op(const X& x, const Y& y) ->\
    decltype(se::make_value(__filter(x).value() op __filter(y).value())) {\
    \
    const auto __x = __filter(x);\
    const auto __y = __filter(y);\
    auto result = se::make_value(__x.value() op __y.value());\
    if(__x.is_symbolic() || __y.is_symbolic()) {\
      result.set_expr(BINARY_EXPR(opname, __x.expr(), __y.expr()));\
    }\
    return result;\
  }\
  template<typename X>\
  const auto operator op(const X& x, const int y) ->\
    decltype(se::make_value(__filter(x).value() op y)) {\
    \
    const auto __x = __filter(x);\
    auto result = se::make_value(__x.value() op y);\
    \
    if(__x.is_symbolic()) {\
      se::OperatorAttr attr = se::OperatorInfo<se::opname>::attr;\
      se::SharedExpr raw_expr = __x.se::AbstractValue::expr();\
      if(se::get_associative_attr(attr) && se::get_commutative_attr(attr) && se::get_identity_attr(attr)) {\
        bool create_partial_nary_expr = false;\
        auto kind = raw_expr->kind();\
        if(kind == se::ANY_EXPR || kind == se::VALUE_EXPR) {\
          create_partial_nary_expr = true;\
        } else if(kind == se::NARY_EXPR) {\
          auto nary_expr = std::dynamic_pointer_cast<se::NaryExpr>(raw_expr);\
          if(nary_expr->attr() == attr) {\
            if(nary_expr->is_partial()) {\
              result.set_aux_value(__x.aux_value() op y);\
              result.set_expr(raw_expr);\
              return result;\
            } else {\
              create_partial_nary_expr = true;\
            }\
          }\
        }\
        if(create_partial_nary_expr) {\
          /* __x.value() must act as the identity element of op. */\
          result.set_aux_value(y);\
          result.set_expr(PARTIAL_EXPR(opname, raw_expr));\
          return result;\
        }\
      }\
      \
      result.set_expr(BINARY_EXPR(opname, __x.expr(), se::Value<int>(y).expr()));\
    }\
    return result;\
  }\
  template<typename Y>\
  const auto operator op(const int x, const Y& y) ->\
    decltype(se::make_value(x op __filter(y).value())) {\
    \
    const auto __y = __filter(y);\
    auto result = se::make_value(x op __y.value());\
    if(__y.is_symbolic()) {\
      result.set_expr(BINARY_EXPR(opname, se::Value<int>(x).expr(), __y.expr()));\
    }\
    return result;\
  }\

OVERLOAD_BINARY_OPERATOR(+, ADD)
OVERLOAD_BINARY_OPERATOR(&&, LAND)
OVERLOAD_BINARY_OPERATOR(||, LOR)
OVERLOAD_BINARY_OPERATOR(==, EQL)
OVERLOAD_BINARY_OPERATOR(<, LSS)

// OVERLOAD_UNARY_OPERATOR(op, opname) is a macro that uses templates and the
// statically overloaded and inlined __filter functions to implement a custom
// C++ unary operator "op" whose reflection identifier is "opname".
//
// Note that none of the template types can ever be native or smart pointers.
#define OVERLOAD_UNARY_OPERATOR(op, opname) \
  template<typename T>\
  const auto operator op(const T& x) ->\
    decltype(se::make_value(op __filter(x).value())) {\
    \
    const auto __x = __filter(x);\
    auto result = se::make_value(op __x.value());\
    if(__x.is_symbolic()) {\
      result.set_expr(se::SharedExpr(new se::UnaryExpr(se::opname, __x.expr())));\
    }\
    return result;\
  }\

OVERLOAD_UNARY_OPERATOR(!, NOT)

#endif /* SE_H_ */
