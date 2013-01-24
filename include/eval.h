// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef EVAL_H_
#define EVAL_H_

#include "op.h"

namespace se {

/// Evaluate built-in arithmetic and boolean expressions
template<Operator op> class Eval;

#define EVAL_BINARY(op, opname) \
  template<>\
  class Eval<opname> {\
  public:\
    template<typename T, typename U, typename V>\
    static inline V eval(const T larg, const U rarg, const V result) {\
      return larg op rarg;\
    }\
  };

#define EVAL_UNARY(op, opname) \
  template<>\
  class Eval<opname> {\
  public:\
    template<typename T, typename U>\
    static inline U eval(const T arg, const U result) {\
      return op arg;\
    }\
  };

EVAL_BINARY(+, ADD)
EVAL_BINARY(&&, LAND)
EVAL_BINARY(||, LOR)
EVAL_BINARY(==, EQL)
EVAL_BINARY(<, LSS)

EVAL_UNARY(!, NOT)

}

#endif /* EVAL_H_ */
