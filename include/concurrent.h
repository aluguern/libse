// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_H_
#define LIBSE_CONCURRENT_H_

#include <utility>
#include <memory>

#include "core/op.h"
#include "core/type.h"

#include "concurrent/event.h"
#include "concurrent/instr.h"
#include "concurrent/recorder.h"
#include "concurrent/var.h"

namespace se {

template<typename T>
inline std::unique_ptr<ReadInstr<T>> alloc_read_instr(const ConcurrentVar<T>& var) {
  const std::shared_ptr<ReadInstr<bool>> condition = path_condition().top();
  std::unique_ptr<ReadEvent<T>> event_ptr(new ReadEvent<T>(var.addr(), condition));
  return std::unique_ptr<ReadInstr<T>>(new BasicReadInstr<T>(std::move(event_ptr)));
}

template<typename T>
inline std::unique_ptr<ReadInstr<typename
  std::enable_if<std::is_arithmetic<T>::value, T>::type>> alloc_read_instr(const T& literal) {
  const std::shared_ptr<ReadInstr<bool>> condition = path_condition().top();
  return std::unique_ptr<ReadInstr<T>>(new LiteralReadInstr<T>(literal, condition));
}

template<typename T> struct UnwrapType<ConcurrentVar<T>> { typedef T base; };

#define CONCURRENT_UNARY_OP(op, opcode) \
  template<typename T>\
  inline auto operator op(std::unique_ptr<ReadInstr<T>> instr) ->\
    std::unique_ptr<ReadInstr<typename ReturnType<opcode, T>::result_type>> {\
    \
    return std::unique_ptr<ReadInstr<typename ReturnType<opcode, T>::result_type>>(\
      new UnaryReadInstr<opcode, T>(std::move(instr)));\
  }\

#define CONCURRENT_BINARY_OP(op, opcode) \
  template<typename T, typename U>\
  inline auto operator op(const T& larg, const U& rarg) ->\
    std::unique_ptr<ReadInstr<typename ReturnType<opcode, typename\
      UnwrapType<T>::base, typename UnwrapType<U>::base>::result_type>> {\
    \
    return alloc_read_instr(larg) op alloc_read_instr(rarg);\
  }\
  template<typename T, typename U>\
  inline auto operator op(std::unique_ptr<ReadInstr<T>> linstr,\
    std::unique_ptr<ReadInstr<U>> rinstr) ->\
    std::unique_ptr<ReadInstr<typename ReturnType<opcode, T, U>::result_type>> {\
    \
    return std::unique_ptr<ReadInstr<typename ReturnType<opcode, T, U>::result_type>>(\
      new BinaryReadInstr<opcode, T, U>(std::move(linstr), std::move(rinstr)));\
  }\

CONCURRENT_UNARY_OP(!, NOT)

CONCURRENT_BINARY_OP(+, ADD)
CONCURRENT_BINARY_OP(&&, LAND)
CONCURRENT_BINARY_OP(||, LOR)
CONCURRENT_BINARY_OP(==, EQL)
CONCURRENT_BINARY_OP(<, LSS)

}

#endif
