// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_ENCODER_H_
#define LIBSE_CONCURRENT_ENCODER_H_

#include <string>

#include <z3++.h>

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

  z3::expr constant(const Event& event) {
    z3::sort sort(context.bv_sort(event.type().bv_size()));
    return context.constant(create_symbol(event), sort);
  }

  z3::expr constant(const ReadEvent<bool>& event) {
    return context.constant(create_symbol(event), context.bool_sort());
  }

  /// Creates a unique Boolean constant for two events
  z3::expr constant(const Event& event_a, const Event& event_b) {
    const std::string name = std::to_string(event_a.event_id()) + ":" +
      std::to_string(event_b.event_id());

    z3::symbol symbol(context.str_symbol(name.c_str()));
    return context.constant(symbol, context.bool_sort());
  }

  template<typename T, size_t N>
  z3::expr constant(const ReadEvent<T[N]>& event) {
    z3::sort domain_sort(context.bv_sort(TypeInfo<size_t>::s_type.bv_size()));
    z3::sort range_sort(context.bv_sort(TypeInfo<T>::s_type.bv_size()));
    z3::sort array_sort(context.array_sort(domain_sort, range_sort));

    return context.constant(create_symbol(event), array_sort);
  }

  template<typename T, typename U, size_t N>
  z3::expr constant(const IndirectWriteEvent<T, U, N>& event) {
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
  z3::expr encode(const DerefReadInstr<T[N], U>& instr, Z3& helper) const {
    return z3::select(instr.memory_ref().encode(*this, helper),
      instr.offset_ref().encode(*this, helper));
  }
};

#define READ_ENCODER_FN \
  encode(const Z3ReadEncoder& encoder, Z3& helper) const {\
    return encoder.encode(*this, helper);\
  }

template<typename T> z3::expr LiteralReadInstr<T>::READ_ENCODER_FN
template<typename T> z3::expr BasicReadInstr<T>::READ_ENCODER_FN

template<Operator op, typename T>
z3::expr UnaryReadInstr<op, T>::READ_ENCODER_FN

template<Operator op, typename T, typename U>
z3::expr BinaryReadInstr<op, T, U>::READ_ENCODER_FN

template<typename T, typename U, size_t N>
z3::expr DerefReadInstr<T[N], U>::READ_ENCODER_FN

/// Encoder for a read instruction in the left-hand side of an assignment
class Z3WriteEncoder {
public:
  Z3WriteEncoder() {}
  
  template<typename T, typename U, size_t N>
  z3::expr encode(const DerefReadInstr<T[N], U>& instr,
    const Z3ReadEncoder& read_encoder,
    const z3::expr& rhs_expr, Z3& helper) const {

    return z3::store(instr.memory_ref().encode(read_encoder, helper),
      instr.offset_ref().encode(read_encoder, helper), rhs_expr);
  }
};

/// Encoder for direct and indirect write events

/// Every member function returns a Z3 expression whose sort is Boolean.
class Z3Encoder {
private:
  const Z3WriteEncoder m_write_encoder;
  const Z3ReadEncoder m_read_encoder;

public:
  Z3Encoder() : m_read_encoder(), m_write_encoder() {}

  template<typename T>
  z3::expr encode(const ReadEvent<T>& event, Z3& helper) const {
    return helper.context.bool_val(false);
  }

  template<typename T>
  z3::expr encode(const DirectWriteEvent<T>& event, Z3& helper) const {
    z3::expr lhs_expr(helper.constant(event));
    z3::expr rhs_expr(event.instr_ref().encode(m_read_encoder, helper));
    return lhs_expr == rhs_expr;
  }

  template<typename T, typename U, size_t N>
  z3::expr encode(const IndirectWriteEvent<T, U, N>& event, Z3& helper) const {
    z3::expr lhs_expr(helper.constant(event));
    z3::expr rhs_expr(event.instr_ref().encode(m_read_encoder, helper));
    return lhs_expr == m_write_encoder.encode(event.deref_instr_ref(),
      m_read_encoder, rhs_expr, helper);
  }
};

#define ENCODER_FN \
  encode(const Z3Encoder& encoder, Z3& helper) const {\
    return encoder.encode(*this, helper);\
  }

template<typename T>
z3::expr ReadEvent<T>::ENCODER_FN

template<typename T>
z3::expr DirectWriteEvent<T>::ENCODER_FN

template<typename T, typename U, size_t N>
z3::expr IndirectWriteEvent<T, U, N>::ENCODER_FN

class Z3OrderEncoder {
private:
  Z3ReadEncoder m_read_encoder;

  z3::expr event_condition(const Event& event, Z3& z3) const {
    if (event.condition_ptr()) {
      return event.condition_ptr()->encode(m_read_encoder, z3);
    }

    return z3.context.bool_val(true);
  }

  typedef std::shared_ptr<Event> EventPtr;
  typedef std::unordered_set<EventPtr> EventPtrSet;

public:
  Z3OrderEncoder() : m_read_encoder() {}

  z3::expr rfe_encode(const MemoryAddrRelation<Event>& relation, Z3& z3) const {
    const MemoryAddrSet& addrs = relation.addrs();
    for (const MemoryAddr& addr : addrs) {
      const std::pair<EventPtrSet, EventPtrSet> result =
        relation.partition(addr);
      const EventPtrSet& read_event_ptrs = result.first;
      const EventPtrSet& write_event_ptrs = result.second;

      z3::expr rfe_expr(z3.context.bool_val(true));
      for (const EventPtr& read_event_ptr : read_event_ptrs) {
        const Event& read_event = *read_event_ptr;
        const z3::expr read_event_condition(event_condition(read_event, z3));

        z3::expr wr_schedules(z3.context.bool_val(false));
        for (const EventPtr& write_event_ptr : write_event_ptrs) {
          const Event& write_event = *write_event_ptr;

          if (write_event.thread_id() == read_event.thread_id()) {
            continue;
          }

          const z3::expr wr_order(z3.clock(write_event) < z3.clock(read_event));
          const z3::expr wr_schedule(z3.constant(write_event, read_event));
          const z3::expr wr_equality(z3.constant(write_event) ==
            z3.constant(read_event));

          wr_schedules = wr_schedules or wr_schedule;
          rfe_expr = rfe_expr and
            implies(wr_schedule, wr_order and wr_equality) and
            implies(wr_order, event_condition(write_event, z3) and
              read_event_condition);
        }

        rfe_expr = rfe_expr and implies(read_event_condition, wr_schedules);
      }
      return rfe_expr;
    }
  }

  z3::expr ws_encode(const MemoryAddrRelation<Event>& relation, Z3& z3) const {
    const MemoryAddrSet& addrs = relation.addrs();

    z3::expr ws_expr(z3.context.bool_val(true));
    for (const MemoryAddr& addr : addrs) {
      const EventPtrSet write_event_ptrs = relation.find(addr,
        WriteEventPredicate::predicate());
      for (const EventPtr& write_event_ptr_x : write_event_ptrs) {
        for (const EventPtr& write_event_ptr_y : write_event_ptrs) {
          const Event& x = *write_event_ptr_x;
          const Event& y = *write_event_ptr_y;

          if (x.thread_id() == y.thread_id()) {
            continue;
          }

          const z3::expr xy_order(z3.clock(x) < z3.clock(y));
          const z3::expr yx_order(z3.clock(y) < z3.clock(x));
          const z3::expr x_condition(event_condition(x, z3));
          const z3::expr y_condition(event_condition(y, z3));

          ws_expr = ws_expr and implies(x_condition and y_condition,
            xy_order or yx_order);
        }
      }
    }

    return ws_expr;
  }
};

}

#endif
