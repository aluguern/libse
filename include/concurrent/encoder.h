// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_ENCODER_H_
#define LIBSE_CONCURRENT_ENCODER_H_

#include <unordered_set>

#include "core/op.h"
#include "concurrent/instr.h"

#include <smt>

//#define __USE_BV__ 1

namespace se {

class Event;

#ifdef __USE_BV__
typedef unsigned short ClockSort;
#else
typedef smt::Int ClockSort;
#endif

class Clock
{
private:
  smt::Term<ClockSort> m_term;

public:
  Clock(const smt::Term<ClockSort>& term)
  : m_term(term) {}

  Clock(const Clock& other)
  : m_term(other.m_term) {}

  Clock(Clock&& other)
  : m_term(std::move(other.m_term)) {}

  smt::Term<smt::Bool> happens_before(
    const Clock& y) const
  {
    return m_term < y.m_term;
  }

  smt::Term<smt::Bool> simultaneous(
    const Clock& y) const
  {
    return m_term == y.m_term;
  }

  smt::Term<smt::Bool> simultaneous_or_happens_before(
    const Clock& y) const
  {
    return m_term <= y.m_term;
  }

  const smt::Term<ClockSort>& term() const
  {
    return m_term;
  }

  Clock& operator=(const Clock& other)
  {
    m_term = other.m_term;
    return *this;
  }
};

class Encoders {
public:
  // logic must support uninterpreted functions and
  // uses bit vectors only if __USE_BV__ is defined
  smt::CVC4Solver solver;

private:
  const std::string m_rf_prefix;
  const std::string m_sup_clock_prefix;
  const std::string m_clock_prefix;
  const std::string m_join_clock_prefix;
  const std::string m_event_prefix;
  const Clock m_epoch;

  friend class ValueEncoder;
  friend class ReadInstrEncoder;

  template<typename T> friend smt::UnsafeTerm ReadEvent<T>::constant(Encoders&) const;
  template<typename T> friend smt::UnsafeTerm DirectWriteEvent<T>::constant(Encoders&) const;

  template<typename T, typename U, size_t N>
  friend smt::UnsafeTerm IndirectWriteEvent<T, U, N>::constant(Encoders&) const;

  unsigned m_join_id;

  std::string create_symbol(const Event& event) {
    return m_event_prefix + std::to_string(event.event_id());
  }

  template<typename T, size_t N>
  smt::UnsafeTerm create_array_constant(const Event& event) {
#ifdef __USE_BV__
    return smt::any<smt::Array<size_t, T>>(create_symbol(event));
#else
    return smt::any<smt::Array<smt::Int, smt::Int>>(create_symbol(event));
#endif
  }

  smt::UnsafeTerm constant(const ReadEvent<bool>& event) {
    return smt::any<smt::Bool>(create_symbol(event));
  }

  template<typename T, size_t N>
  smt::UnsafeTerm constant(const ReadEvent<T[N]>& event) {
    return create_array_constant<T, N>(event);
  }

  template<typename T, size_t N>
  smt::UnsafeTerm constant(const WriteEvent<T[N]>& event) {
    return create_array_constant<T, N>(event);
  }

  template<typename T, typename U, size_t N>
  smt::UnsafeTerm constant(const IndirectWriteEvent<T, U, N>& event) {
    return create_array_constant<T, N>(event);
  }

public:
  Encoders()
#ifdef __USE_BV__
  : solver(smt::QF_AUFBV_LOGIC),
#else
  : solver(smt::QF_AUFLIA_LOGIC),
#endif
    m_rf_prefix("rf_"),
    m_sup_clock_prefix("sup-clock_"),
    m_clock_prefix("clock_"),
    m_join_clock_prefix("join-clock_"),
    m_event_prefix("event_"),
#ifndef __USE_MATRIX
    m_epoch(smt::literal<ClockSort>(0)),
#endif
    m_join_id(0) {}

  void reset() {
    solver.reset();
  }

  /// Creates a Z3 constant according to the event's \ref Event::type() "type"
  smt::UnsafeTerm constant(const Event& event) {
#ifdef __USE_BV__
    const smt::Sort& sort = smt::bv_sort(event.type().is_signed(), event.type().bv_size());
    const smt::UnsafeDecl decl(create_symbol(event), sort);
#else
    const smt::UnsafeDecl decl(create_symbol(event), smt::internal::sort<smt::Int>());
#endif
    return smt::constant(decl);
  }

  Clock join_clocks(
    const Clock& x,
    const Clock& y)
  {
#ifndef __USE_MATRIX__
    const std::string join_name = m_join_clock_prefix + std::to_string(m_join_id++);
    const Clock join_clock(smt::any<ClockSort>(join_name));
    solver.add(m_epoch.happens_before(join_clock));
    solver.add(x.happens_before(join_clock) && y.happens_before(join_clock));
    return join_clock;
#endif
  }

  /// Equality between write event and read event applied to function `rf`

  /// \returns `w == rf(r)`, i.e. `r` reads from `w`
  smt::UnsafeTerm rf(const Event& write_event, const Event& read_event) {
    assert(write_event.is_write());
    assert(read_event.is_read());

    const smt::Term<ClockSort> rf_clock =
      smt::any<ClockSort>(m_rf_prefix + create_symbol(read_event));
    return write_event.event_id() == rf_clock;
  }

  /// Unique clock constraint for an event
  Clock clock(const Event& event) {
#ifndef __USE_MATRIX__
    const Clock clock(smt::any<ClockSort>(m_clock_prefix + create_symbol(event)));
    solver.add(m_epoch.happens_before(clock));
    return clock;
#endif
  }

  void transitivity(const std::unordered_set<std::shared_ptr<Event>>& event_ptrs)
  {
/*    for (const std::shared_ptr<Event>& x : event_ptrs) {
      for (const std::shared_ptr<Event>& y : event_ptrs) {
        for (const std::shared_ptr<Event>& z : event_ptrs) {
          solver.add(smt::implies(clock(*x) <= clock(*y) and clock(*y) <= clock(*z),
            clock(*x) <= clock(*z)));
        }
      }
    }
*/
  }

  /// Individual array element literal
  template<typename T, size_t N = std::extent<T>::value,
    class = typename std::enable_if<std::is_array<T>::value and 0 < N>::type>
  smt::UnsafeTerm literal(const LiteralReadInstr<T>& instr) {

    typedef typename std::remove_extent<T>::type ElementType;
    return literal(LiteralReadInstr<ElementType>(instr.element_literal()));
  }

  smt::UnsafeTerm literal(const LiteralReadInstr<bool>& instr) {
    return smt::literal<smt::Bool>(instr.literal());
  }

  template<typename T, class = typename std::enable_if<
    std::is_arithmetic<T>::value>::type>
  smt::UnsafeTerm literal(const LiteralReadInstr<T>& instr) {
#if __USE_BV__
    return smt::literal<T>(instr.literal());
#else
    return smt::literal<smt::Int>(instr.literal());
#endif
  }

  /// Find upper bound of `{clock(e) | e in E and clock(e) < clock(r)}`
  Clock sup_clock(const Event& read_event) {
    assert(read_event.is_read());

    return Clock(smt::any<ClockSort>(m_sup_clock_prefix + create_symbol(read_event)));
  }
};

template<Opcode opcode, typename T>
struct Z3Identity {
  static smt::UnsafeTerm constant();
};

template<>
struct Z3Identity<LAND, bool> {
  static smt::UnsafeTerm constant() {
    return smt::literal<smt::Bool>(true);
  }
};

/// Encoder for read instructions 
class ReadInstrEncoder {
private:

public:
  ReadInstrEncoder() {}

  template<typename T>
  smt::UnsafeTerm encode(const LiteralReadInstr<T>& instr, Encoders& helper) const {
    return helper.literal(instr);
  }

  template<typename T>
  smt::UnsafeTerm encode(const BasicReadInstr<T>& instr, Encoders& helper) const {
    return helper.constant(*instr.event_ptr());
  }

  template<Opcode opcode, typename T>
  smt::UnsafeTerm encode(const UnaryReadInstr<opcode, T>& instr, Encoders& helper) const {
    return Eval<opcode>::eval(instr.operand_ref().encode(*this, helper));
  }

  template<Opcode opcode, typename T, typename U>
  smt::UnsafeTerm encode(const BinaryReadInstr<opcode, T, U>& instr, Encoders& helper) const {
    return Eval<opcode>::eval(instr.loperand_ref().encode(*this, helper),
      instr.roperand_ref().encode(*this, helper));
  }

  template<Opcode opcode, typename T>
  smt::UnsafeTerm encode(const NaryReadInstr<opcode, T>& instr, Encoders& helper) const {
    smt::UnsafeTerm nary_expr = Z3Identity<opcode, T>::constant();
    for (const std::shared_ptr<ReadInstr<T>>& operand_ptr : instr.operand_ptrs()) {
      nary_expr = Eval<opcode>::eval(nary_expr, operand_ptr->encode(*this, helper));
    }
    return nary_expr;
  }

  template<typename T, typename U, size_t N>
  smt::UnsafeTerm encode(const DerefReadInstr<T[N], U>& instr, Encoders& helper) const {
    return smt::select(instr.memory_ref().encode(*this, helper),
      instr.offset_ref().encode(*this, helper));
  }
};

#define READ_ENCODER_FN_DEF \
  encode(const ReadInstrEncoder& encoder, Encoders& helper) const {\
    return encoder.encode(*this, helper);\
  }

template<typename T> smt::UnsafeTerm LiteralReadInstr<T>::READ_ENCODER_FN_DEF
template<typename T, size_t N> smt::UnsafeTerm LiteralReadInstr<T[N]>::READ_ENCODER_FN_DEF
template<typename T> smt::UnsafeTerm BasicReadInstr<T>::READ_ENCODER_FN_DEF

template<Opcode opcode, typename T>
smt::UnsafeTerm UnaryReadInstr<opcode, T>::READ_ENCODER_FN_DEF

template<Opcode opcode, typename T, typename U>
smt::UnsafeTerm BinaryReadInstr<opcode, T, U>::READ_ENCODER_FN_DEF

template<Opcode opcode, typename T>
smt::UnsafeTerm NaryReadInstr<opcode, T>::READ_ENCODER_FN_DEF

template<typename T, typename U, size_t N>
smt::UnsafeTerm DerefReadInstr<T[N], U>::READ_ENCODER_FN_DEF

/// Encoder for the values of direct and indirect write events

/// Every `encode_eq(...)` member function returns a Z3 expression whose sort
/// is Boolean.
class ValueEncoder {
private:
  const ReadInstrEncoder m_read_encoder;

  template<typename T, typename U, size_t N>
  smt::UnsafeTerm encode_indirect_write(const DerefReadInstr<T[N], U>& instr,
    const ReadInstrEncoder& read_encoder,
    const smt::UnsafeTerm& rhs_expr, Encoders& helper) const {

    return smt::store(instr.memory_ref().encode(read_encoder, helper),
      instr.offset_ref().encode(read_encoder, helper), rhs_expr);
  }

public:
  ValueEncoder() : m_read_encoder() {}

  template<typename T>
  smt::UnsafeTerm encode_eq(const ReadEvent<T>& event, Encoders& helper) const {
    return smt::literal<smt::Bool>(false);
  }

  smt::UnsafeTerm encode_eq(const SyncEvent& event, Encoders& helper) const {
    return smt::literal<smt::Bool>(true);
  }

  template<typename T>
  smt::UnsafeTerm encode_eq(const DirectWriteEvent<T>& event, Encoders& helper) const {
    smt::UnsafeTerm lhs_expr(helper.constant(event));
    smt::UnsafeTerm rhs_expr(event.instr_ref().encode(m_read_encoder, helper));
    return lhs_expr == rhs_expr;
  }

  template<typename T, size_t N>
  smt::UnsafeTerm encode_eq(const DirectWriteEvent<T[N]>& event, Encoders& helper) const {
    smt::UnsafeTerm lhs_expr(helper.constant(event));
    smt::UnsafeTerm init_expr(event.instr_ref().encode(m_read_encoder, helper));
    smt::UnsafeTerm and_expr(smt::literal<smt::Bool>(true));
    for (size_t i = 0; i < N; i++) {
#if __USE_BV__
      and_expr = and_expr && (smt::select(lhs_expr, smt::literal<size_t>(i)) == init_expr);
#else
      and_expr = and_expr && (smt::select(lhs_expr, smt::literal<smt::Int>(i)) == init_expr);
#endif
    }
    return and_expr;
  }

  template<typename T, typename U, size_t N>
  smt::UnsafeTerm encode_eq(const IndirectWriteEvent<T, U, N>& event, Encoders& helper) const {
    smt::UnsafeTerm lhs_expr(helper.constant(event));
    smt::UnsafeTerm rhs_expr(event.instr_ref().encode(m_read_encoder, helper));
    return lhs_expr == encode_indirect_write(event.deref_instr_ref(),
      m_read_encoder, rhs_expr, helper);
  }

  template<typename T>
  smt::UnsafeTerm encode_eq(std::unique_ptr<ReadInstr<T>> instr_ptr, Encoders& encoders) const {
    return instr_ptr->encode(m_read_encoder, encoders);
  }
};

#define VALUE_ENCODER_FN_DEF \
  encode_eq(const ValueEncoder& encoder, Encoders& helper) const {\
    return encoder.encode_eq(*this, helper);\
  }

#define CONSTANT_ENCODER_FN_DEF \
  constant(Encoders& helper) const { return helper.constant(*this); }

template<typename T>
smt::UnsafeTerm ReadEvent<T>::VALUE_ENCODER_FN_DEF

template<typename T>
smt::UnsafeTerm ReadEvent<T>::CONSTANT_ENCODER_FN_DEF

template<typename T>
smt::UnsafeTerm DirectWriteEvent<T>::VALUE_ENCODER_FN_DEF

template<typename T>
smt::UnsafeTerm DirectWriteEvent<T>::CONSTANT_ENCODER_FN_DEF

template<typename T, typename U, size_t N>
smt::UnsafeTerm IndirectWriteEvent<T, U, N>::VALUE_ENCODER_FN_DEF

template<typename T, typename U, size_t N>
smt::UnsafeTerm IndirectWriteEvent<T, U, N>::CONSTANT_ENCODER_FN_DEF













}

#endif
