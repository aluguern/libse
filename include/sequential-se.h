// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef SE_H_
#define SE_H_

#include "value.h"
#include "var.h"
#include "if.h"
#include "loop.h"

// Internal function to guide the type inference of template functions that
// overload built-in operators such as addition, multiplication etc.
template<typename T>
static inline const se::Value<T>& __filter(const se::Var<T>& var) {
  return var.get_value();
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
    se::OperatorTraits<se::opname>::attr, (expr)))

// BINARY_EXPR is a strictly internal macro that instantiates a new binary
// expression for the given binary operator enum value and operands.
#define BINARY_EXPR(opname, x_expr, y_expr) \
  se::SharedExpr(new se::NaryExpr(se::opname,\
    se::OperatorTraits<se::opname>::attr, (x_expr), (y_expr)))

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
    decltype(se::make_value(__filter(x).get_value() op __filter(y).get_value())) {\
    \
    const auto __x = __filter(x);\
    const auto __y = __filter(y);\
    auto result = se::make_value(__x.get_value() op __y.get_value());\
    if(__x.is_symbolic() || __y.is_symbolic()) {\
      result.set_expr(BINARY_EXPR(opname, __x.get_expr(), __y.get_expr()));\
    }\
    return result;\
  }\
  template<typename X>\
  const auto operator op(const X& x, const int y) ->\
    decltype(se::make_value(__filter(x).get_value() op y)) {\
    \
    const auto __x = __filter(x);\
    auto result = se::make_value(__x.get_value() op y);\
    \
    if(__x.is_symbolic()) {\
      se::OperatorAttr attr = se::OperatorTraits<se::opname>::attr;\
      se::SharedExpr raw_expr = __x.se::GenericValue::get_expr();\
      if(se::get_associative_attr(attr) && se::get_commutative_attr(attr) && se::get_identity_attr(attr)) {\
        bool create_partial_nary_expr = false;\
        auto kind = raw_expr->get_kind();\
        if(kind == se::ANY_EXPR || kind == se::VALUE_EXPR) {\
          create_partial_nary_expr = true;\
        } else if(kind == se::NARY_EXPR) {\
          auto nary_expr = std::dynamic_pointer_cast<se::NaryExpr>(raw_expr);\
          if(nary_expr->get_attr() == attr) {\
            if(nary_expr->is_partial()) {\
              result.set_aux_value(__x.get_aux_value() op y);\
              result.set_expr(raw_expr);\
              return result;\
            } else {\
              create_partial_nary_expr = true;\
            }\
          }\
        }\
        if(create_partial_nary_expr) {\
          /* __x.get_value() must act as the identity element of op. */\
          result.set_aux_value(y);\
          result.set_expr(PARTIAL_EXPR(opname, raw_expr));\
          return result;\
        }\
      }\
      \
      result.set_expr(BINARY_EXPR(opname, __x.get_expr(), se::Value<int>(y).get_expr()));\
    }\
    return result;\
  }\
  template<typename Y>\
  const auto operator op(const int x, const Y& y) ->\
    decltype(se::make_value(x op __filter(y).get_value())) {\
    \
    const auto __y = __filter(y);\
    auto result = se::make_value(x op __y.get_value());\
    if(__y.is_symbolic()) {\
      result.set_expr(BINARY_EXPR(opname, se::Value<int>(x).get_expr(), __y.get_expr()));\
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
    decltype(se::make_value(op __filter(x).get_value())) {\
    \
    const auto __x = __filter(x);\
    auto result = se::make_value(op __x.get_value());\
    if(__x.is_symbolic()) {\
      result.set_expr(se::SharedExpr(new se::UnaryExpr(se::opname, __x.get_expr())));\
    }\
    return result;\
  }\

OVERLOAD_UNARY_OPERATOR(!, NOT)

#endif /* SE_H_ */
