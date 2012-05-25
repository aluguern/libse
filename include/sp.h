#ifndef SP_H_
#define SP_H_

#include "value.h"
#include "any.h"
#include "var.h"
#include "loop.h"

// Internal function to guide the type inference of template functions that
// overload built-in operators such as addition, multiplication etc.
template<typename T>
static inline const sp::Value<T>& __filter(const sp::Var<T>& var) {
  return var.get_value();
}

// Internal function to guide the type inference of template functions that
// overload built-in operators such as addition, multiplication etc.
template<typename T>
static inline const sp::Value<T>& __filter(const sp::Value<T>& value) {
  return value;
}

// OVERLOAD_BINARY_OPERATOR(op, opname) is a macro that uses templates and the
// statically overloaded and inlined __filter functions to implement a custom
// C++ binary operator "op" whose reflection identifier is "opname".
//
// Note that none of the template types can ever be native or smart pointers.
#define OVERLOAD_BINARY_OPERATOR(op, opname) \
  template<typename X, typename Y>\
  const auto operator op(const X& x, const Y& y) ->\
    decltype(sp::reflect(__filter(x).get_value() op __filter(y).get_value())) {\
    \
    const auto __x = __filter(x);\
    const auto __y = __filter(y);\
    auto result = sp::reflect(__x.get_value() op __y.get_value());\
    if(__x.is_symbolic() || __y.is_symbolic()) {\
      result.set_expr(sp::SharedExpr(new sp::BinaryExpr(\
        __x.get_expr(), __y.get_expr(), sp::opname)));\
    }\
    return result;\
  }\
  template<typename X>\
  const auto operator op(const X& x, const int y) ->\
    decltype(sp::reflect(__filter(x).get_value() op y)) {\
    \
    const auto __x = __filter(x);\
    auto result = sp::reflect(__x.get_value() op y);\
    if(__x.is_symbolic()) {\
      result.set_expr(sp::SharedExpr(new sp::BinaryExpr(\
        __x.get_expr(), sp::Value<int>(y).get_expr(), sp::opname)));\
    }\
    return result;\
  }\
  template<typename Y>\
  const auto operator op(const int x, const Y& y) ->\
    decltype(sp::reflect(x + __filter(y).get_value())) {\
    \
    const auto __y = __filter(y);\
    auto result = sp::reflect(x op __y.get_value());\
    if(__y.is_symbolic()) {\
      result.set_expr(sp::SharedExpr(new sp::BinaryExpr(\
        sp::Value<int>(x).get_expr(), __y.get_expr(), sp::opname)));\
    }\
    return result;\
  }\

OVERLOAD_BINARY_OPERATOR(+, ADD)
OVERLOAD_BINARY_OPERATOR(<, LSS)

// OVERLOAD_UNARY_OPERATOR(op, opname) is a macro that uses templates and the
// statically overloaded and inlined __filter functions to implement a custom
// C++ unary operator "op" whose reflection identifier is "opname".
//
// Note that none of the template types can ever be native or smart pointers.
#define OVERLOAD_UNARY_OPERATOR(op, opname) \
  template<typename T>\
  const auto operator op(const T& x) ->\
    decltype(sp::reflect(op __filter(x).get_value())) {\
    \
    const auto __x = __filter(x);\
    auto result = sp::reflect(op __x.get_value());\
    if(__x.is_symbolic()) {\
      result.set_expr(sp::SharedExpr(new sp::UnaryExpr(__x.get_expr(), sp::opname)));\
    }\
    return result;\
  }\

OVERLOAD_UNARY_OPERATOR(!, NOT)

#endif /* SP_H_ */
