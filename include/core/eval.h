// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef EVAL_H_
#define EVAL_H_

#include "core/op.h"

namespace se {

/// Evaluate built-in arithmetic and boolean expressions
template<Operator op> class Eval;

#define EVAL_UNARY(op, opname) \
  template<>\
  class Eval<opname> {\
  public:\
    template<typename T>\
    static inline auto eval(const T arg) ->\
      decltype(op arg) { return op arg; }\
    \
    template<typename T>\
    static constexpr auto const_eval(const T arg) ->\
      decltype(op arg) { return op arg; }\
  };

#define EVAL_BINARY(op, opname) \
  template<>\
  class Eval<opname> {\
  public:\
    template<typename T, typename U>\
    static inline auto eval(const T larg, const U rarg) ->\
      decltype(larg + rarg) { return larg op rarg; }\
    \
    template<typename T, typename U>\
    static constexpr auto const_eval(const T larg, const U rarg) ->\
      decltype(larg op rarg) { return larg op rarg; }\
  };

EVAL_UNARY(!, NOT)

EVAL_BINARY(+, ADD)
EVAL_BINARY(&&, LAND)
EVAL_BINARY(||, LOR)
EVAL_BINARY(==, EQL)
EVAL_BINARY(<, LSS)

}

#endif /* EVAL_H_ */
