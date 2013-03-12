// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_ENCODER_H_
#define LIBSE_CONCURRENT_ENCODER_H_

#include <z3++.h>

#include <string>

#include "concurrent/instr.h"
#include "concurrent/relation.h"

namespace se {

class Z3 {
public:
  z3::context context;
  z3::solver solver;

private:
  z3::symbol create_symbol(const Event& event) {
    return context.int_symbol(event.event_id());
  }

public:
  Z3() : context(), solver(context) {}

  template<typename T, class = typename std::enable_if<
    std::is_arithmetic<T>::value>::type>
  z3::expr literal(const LiteralReadInstr<T>& instr) {
    const char* str = std::to_string(instr.literal()).c_str();
    return context.bv_val(str, TypeInfo<T>::s_type.bv_size());
  }

  z3::expr literal(const LiteralReadInstr<bool>& instr) {
    return context.bool_val(instr.literal());
  }

  template<typename T>
  z3::expr constant(const ReadEvent<T>& event) {
    z3::sort sort(context.bv_sort(event.type().bv_size()));
    return context.constant(create_symbol(event), sort);
  }

  template<typename T, size_t N>
  z3::expr constant(const ReadEvent<T[N]>& event) {
    z3::sort domain_sort(context.bv_sort(TypeInfo<size_t>::s_type.bv_size()));
    z3::sort range_sort(context.bv_sort(TypeInfo<T>::s_type.bv_size()));
    z3::sort array_sort(context.array_sort(domain_sort, range_sort));

    return context.constant(create_symbol(event), array_sort);
  }

  z3::expr clock(const Event& event) {
    z3::sort sort(context.int_sort());
    z3::expr clock(context.constant(create_symbol(event), sort));
    solver.add(clock > 0);
    return clock;
  }
};

/// Encoder for read instructions 
class Z3ReadEncoder {
public:
  Z3ReadEncoder() {}

  template<typename T>
  z3::expr encode(const LiteralReadInstr<T>& instr, Z3& helper) const {
    return helper.literal(instr);
  }

  template<typename T>
  z3::expr encode(const BasicReadInstr<T>& instr, Z3& helper) const {
    return helper.constant(*instr.event_ptr());
  }

  template<Operator op, typename T>
  z3::expr encode(const UnaryReadInstr<op, T>& instr, Z3& helper) const {
    return Eval<op>::eval(instr.operand_ref().encode(*this, helper));
  }

  template<Operator op, typename T, typename U>
  z3::expr encode(const BinaryReadInstr<op, T, U>& instr, Z3& helper) const {
    return Eval<op>::eval(instr.loperand_ref().encode(*this, helper),
      instr.roperand_ref().encode(*this, helper));
  }

  template<typename T, typename U, size_t N>
  z3::expr encode(const DereferenceReadInstr<T[N], U>& instr, Z3& helper) const {
    return z3::select(instr.memory_ref().encode(*this, helper),
      instr.offset_ref().encode(*this, helper));
  }
};

#define ENCODE_FN \
  encode(const Z3ReadEncoder& encoder, Z3& helper) const {\
    return encoder.encode(*this, helper);\
  }

template<typename T> z3::expr LiteralReadInstr<T>::ENCODE_FN
template<typename T> z3::expr BasicReadInstr<T>::ENCODE_FN

template<Operator op, typename T>
z3::expr UnaryReadInstr<op, T>::ENCODE_FN

template<Operator op, typename T, typename U>
z3::expr BinaryReadInstr<op, T, U>::ENCODE_FN

template<typename T, typename U, size_t N>
z3::expr DereferenceReadInstr<T[N], U>::ENCODE_FN

}

#endif
