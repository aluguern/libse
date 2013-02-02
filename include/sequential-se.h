// Copyright 2012-2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef SE_H_
#define SE_H_

#include "value.h"
#include "var.h"
#include "if.h"
#include "loop.h"
#include "instr.h"

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
static inline const se::Value<T>& __value(const se::Var<T>& var) {
  return var.value();
}

// Internal function to guide the type inference of template functions that
// overload built-in operators such as addition, multiplication etc.
template<typename T>
static inline const se::Value<T>& __value(const se::Value<T>& value) {
  return value;
}

// OVERLOAD_BINARY_OPERATOR(op, opname) is a macro that uses templates and the
// statically overloaded and inlined __value functions to implement a custom
// C++ binary operator "op" whose reflection identifier is "opname". These
// binary operators are assumed to be associative and commutative. If this is
// not the case (e.g. IEEE 754 floating-point addition), function template
// specializations should be used to deal with these cases separately.
//
// Note that none of the template types can ever be native or smart pointers.
#define OVERLOAD_BINARY_OPERATOR(op, opname) \
  template<typename T, typename U>\
  auto operator op(const T& larg, const U& rarg) ->\
    decltype(se::make_value(__value(larg).data() op __value(rarg).data())) {\
    \
    const auto larg_value = __value(larg);\
    const auto rarg_value = __value(rarg);\
    auto ret = se::make_value(larg_value.data() op rarg_value.data());\
    se::Instr<se::opname>::exec(larg_value, rarg_value, ret);\
    return ret;\
  }\
  template<typename T>\
  auto operator op(const T& larg, const int rarg) ->\
    decltype(se::make_value(__value(larg).data() op rarg)) {\
    \
    const auto larg_value = __value(larg);\
    auto ret = se::make_value(larg_value.data() op rarg);\
    se::Instr<se::opname>::exec(larg_value, rarg, ret);\
    return ret;\
  }\
  template<typename T>\
  auto operator op(const int larg, const T& rarg) ->\
    decltype(se::make_value(larg op __value(rarg).data())) {\
    \
    const auto rarg_value = __value(rarg);\
    auto ret = se::make_value(larg op rarg_value.data());\
    se::Instr<se::opname>::exec(larg, rarg_value, ret);\
    return ret;\
  }

OVERLOAD_BINARY_OPERATOR(+, ADD)
OVERLOAD_BINARY_OPERATOR(&&, LAND)
OVERLOAD_BINARY_OPERATOR(||, LOR)
OVERLOAD_BINARY_OPERATOR(==, EQL)
OVERLOAD_BINARY_OPERATOR(<, LSS)

// OVERLOAD_UNARY_OPERATOR(op, opname) is a macro that uses templates and the
// statically overloaded and inlined __value functions to implement a custom
// C++ unary operator "op" whose reflection identifier is "opname".
//
// Note that none of the template types can ever be native or smart pointers.
#define OVERLOAD_UNARY_OPERATOR(op, opname) \
  template<typename T>\
  auto operator op(const T& arg) ->\
    decltype(se::make_value(op __value(arg).data())) {\
    \
    const auto arg_value = __value(arg);\
    auto ret = se::make_value(op arg_value.data());\
    se::Instr<se::opname>::exec(arg_value, ret);\
    return ret;\
  }

OVERLOAD_UNARY_OPERATOR(!, NOT)

#endif /* SE_H_ */
