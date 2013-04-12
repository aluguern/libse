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
#include "concurrent/encoder_c0.h"
#include "concurrent/recorder.h"
#include "concurrent/thread.h"
#include "concurrent/var.h"

namespace se {

template<typename T>
inline std::unique_ptr<ReadInstr<T>> alloc_read_instr(const LocalVar<T>& local_var) {
  return std::unique_ptr<ReadInstr<T>>(new BasicReadInstr<T>(
    local_var.read_event_ptr()));
}

template<typename T>
inline std::unique_ptr<ReadInstr<T>> alloc_read_instr(const SharedVar<T>& shared_var) {
  return std::unique_ptr<ReadInstr<T>>(new BasicReadInstr<T>(
    make_read_event<T>(shared_var.tag())));
}

template<typename Range, typename Domain, size_t N>
inline std::unique_ptr<ReadInstr<Range>> alloc_read_instr(
  SharedMemory<Range, Domain, N>&& shared_memory) {

  return std::move(shared_memory.deref_instr_ptr());
}

template<typename Range, typename Domain, size_t N>
inline std::unique_ptr<ReadInstr<Range>> alloc_read_instr(
  LocalMemory<Range, Domain, N>&& local_memory) {

  return std::move(local_memory.deref_instr_ptr());
}

template<typename T>
inline std::unique_ptr<ReadInstr<typename
  std::enable_if<std::is_arithmetic<T>::value, T>::type>>
  alloc_read_instr(const T& literal) {

  return std::unique_ptr<ReadInstr<T>>(new LiteralReadInstr<T>(
    literal, ThisThread::recorder().block_condition_ptr()));
}

template<typename T> struct UnwrapType<LocalVar<T>> { typedef T base; };
template<typename T> struct UnwrapType<SharedVar<T>> { typedef T base; };

template<typename Range, typename Domain, size_t N>
struct UnwrapType<LocalMemory<Range, Domain, N>> { typedef Range base; };

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
  inline auto operator op(T&& larg, U&& rarg) ->\
    std::unique_ptr<ReadInstr<typename ReturnType<opcode, typename\
      UnwrapType<T>::base, typename UnwrapType<U>::base>::result_type>> {\
    \
    return alloc_read_instr(std::move(larg)) op alloc_read_instr(std::move(rarg));\
  }\
  template<typename T, typename U>\
  inline auto operator op(std::unique_ptr<ReadInstr<T>> larg, U&& rarg) ->\
    std::unique_ptr<ReadInstr<typename ReturnType<opcode, T,\
      typename UnwrapType<U>::base>::result_type>> {\
    \
    return std::move(larg) op alloc_read_instr(std::move(rarg));\
  }\
  template<typename T, typename U>\
  inline auto operator op(T&& larg, std::unique_ptr<ReadInstr<U>> rarg) ->\
    std::unique_ptr<ReadInstr<typename ReturnType<opcode,\
      typename UnwrapType<U>::base, U>::result_type>> {\
    \
    return alloc_read_instr(std::move(larg)) op std::move(rarg);\
  }\
  template<typename T, typename U>\
  inline auto operator op(const T& larg, const U& rarg) ->\
    std::unique_ptr<ReadInstr<typename ReturnType<opcode, typename\
      UnwrapType<T>::base, typename UnwrapType<U>::base>::result_type>> {\
    \
    return alloc_read_instr(larg) op alloc_read_instr(rarg);\
  }\
  template<typename T, typename U>\
  inline auto operator op(std::unique_ptr<ReadInstr<T>> larg, const U& rarg) ->\
    std::unique_ptr<ReadInstr<typename ReturnType<opcode, T,\
      typename UnwrapType<U>::base>::result_type>> {\
    \
    return std::move(larg) op alloc_read_instr(rarg);\
  }\
  template<typename T, typename U>\
  inline auto operator op(const T& larg, std::unique_ptr<ReadInstr<U>> rarg) ->\
    std::unique_ptr<ReadInstr<typename ReturnType<opcode,\
      typename UnwrapType<U>::base, U>::result_type>> {\
    \
    return alloc_read_instr(larg) op std::move(rarg);\
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
CONCURRENT_UNARY_OP(-, SUB)

CONCURRENT_BINARY_OP(+, ADD)
CONCURRENT_BINARY_OP(-, SUB)
CONCURRENT_BINARY_OP(&&, LAND)
CONCURRENT_BINARY_OP(||, LOR)
CONCURRENT_BINARY_OP(==, EQL)
CONCURRENT_BINARY_OP(<, LSS)

}

#endif
