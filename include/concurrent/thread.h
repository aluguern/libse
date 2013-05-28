// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_THREAD_H_
#define LIBSE_CONCURRENT_THREAD_H_

#include <stack>

#include "concurrent/zone.h"
#include "concurrent/event.h"
#include "concurrent/encoder_c0.h"
#include "concurrent/slice.h"

namespace se {

class Thread;

namespace ThisThread {
  /// Current thread identifier
  ThreadId thread_id();

  /// What thread spawned this current one as a child?

  /// \returns nullptr if this is the main thread
  const Thread* parent_thread_ptr();

  /// Conjunction of conditions along per-thread slice
  const std::shared_ptr<ReadInstr<bool>> path_condition_ptr();

  /// Adds path condition to current per-thread slice 
  void begin_block(const std::shared_ptr<ReadInstr<bool>>&);

  /// Removes latest path condition from per-thread slice
  void end_block();
}

/// Symbolic thread for the analysis of concurrent C++ code
class Thread {
private:
  friend class Threads;
  friend const Thread* ThisThread::parent_thread_ptr();

  typedef std::shared_ptr<ReadInstr<bool>> ConditionPtr;
  typedef std::forward_list<ConditionPtr> ConditionPtrs;

  static Z3C0 s_z3;
  static const std::shared_ptr<ReadInstr<bool>> s_true_condition_ptr;
  static ThreadId s_next_thread_id;

  // unique thread identifier
  const ThreadId m_thread_id;

  // nullptr if and only if this is the main thread
  Thread* m_parent_thread_ptr;

  // can be null
  std::shared_ptr<SendEvent> m_send_event_ptr;

  // per-thread path condition
  size_t m_condition_ptrs_size;
  ConditionPtrs m_condition_ptrs;
  std::stack<ConditionPtr> m_path_condition_ptr_cache;

  // \internal called by Threads::begin_thread()
  Thread(Thread* parent_thread_ptr) :
    m_thread_id(s_next_thread_id++),
    m_parent_thread_ptr(parent_thread_ptr),
    m_send_event_ptr(nullptr),
    m_condition_ptrs_size(0),
    m_condition_ptrs(),
    m_path_condition_ptr_cache() {}

  Thread* parent_thread_ptr() const {
    return m_parent_thread_ptr;
  }

public:
  static Z3C0& z3() {
    return s_z3;
  }

  /// Encode all threads as an SMT formula
  ///
  /// \returns is there at least one error condition to check?
  static bool encode();

  /// A satisfiable condition exposes a (concurrency) bug
  static void error(std::unique_ptr<ReadInstr<bool>> condition_ptr);

  /// Symbolically spawn `f(args...)` as a new thread of execution

  /// \param f non-member function to be executed as a new symbolic thread
  /// \param args arguments \ref std::forward "forwarded" to `f`
  ///
  /// The return value of `f` is always ignored.
  ///
  /// \pre: There must exist a main thread
  template<typename Function, typename... Args>
  explicit Thread(Function&& f, Args&&... args);

  Thread(const Thread&) = delete;
  Thread(Thread&& other) :
    m_thread_id(other.m_thread_id),
    m_parent_thread_ptr(other.m_parent_thread_ptr),
    m_send_event_ptr(std::move(other.m_send_event_ptr)),
    m_condition_ptrs_size(other.m_condition_ptrs_size),
    m_condition_ptrs(std::move(other.m_condition_ptrs)) {}

  ThreadId thread_id() const {
    return m_thread_id;
  }

  void begin_block(const std::shared_ptr<ReadInstr<bool>>& condition_ptr);
  void end_block();

  std::shared_ptr<ReadInstr<bool>> path_condition_ptr();

  template<typename T>
  std::shared_ptr<DirectWriteEvent<T>> instr(const Zone& zone,
    std::unique_ptr<ReadInstr<T>> instr_ptr);

  template<typename T, typename U, size_t N>
  std::shared_ptr<IndirectWriteEvent<T, U, N>> instr(const Zone& zone,
    std::unique_ptr<DerefReadInstr<T[N], U>> deref_instr_ptr,
    std::unique_ptr<ReadInstr<T>> instr_ptr);

  void join() noexcept;
};

/// \internal Thread helper singleton
class Threads {
private:
  static Threads s_singleton;

  Slice m_slice;
  std::stack<Thread> m_thread_stack;

  // nullptr if and only if this is the main thread
  Thread* m_current_thread_ptr;

  // Must only be accessed before Z3C0 solver is deallocated
  std::forward_list<z3::expr> m_error_exprs;

private:
  Threads() :
    m_slice(),
    m_thread_stack(),
    m_current_thread_ptr(nullptr),
    m_error_exprs() {

    internal_reset(0, 0);
  }

  // Clears the entire thread stack and restarts recording the main thread
  void internal_reset(unsigned next_event_id, unsigned next_zone) {
    Event::reset_id(next_event_id);
    Zone::reset(next_zone);

    m_slice.reset();
    while (!m_thread_stack.empty()) { 
      m_thread_stack.pop();
    }

    m_current_thread_ptr = nullptr;
    assert(m_error_exprs.empty());
  }

  // thread_ptr can be nullptr
  static void set_current_thread_ptr(Thread* thread_ptr) {
    s_singleton.m_current_thread_ptr = thread_ptr;
  }

public:
  static Slice& slice() {
    return s_singleton.m_slice;
  }

  /// \internal Modifiable reference to the current thread

  /// \pre: Threads::begin_thread(const Thread&) must have been called
  static Thread& current_thread() {
    assert(s_singleton.m_current_thread_ptr != nullptr);
    return *s_singleton.m_current_thread_ptr;
  }

  /// Erase any previous thread recordings
  static void reset(unsigned next_event_id = 0, unsigned next_zone = 0) {
    return s_singleton.internal_reset(next_event_id, next_zone);
  }

  /// Start recording a new thread of execution
  static void begin_thread() {
    s_singleton.m_thread_stack.push(Thread(s_singleton.m_current_thread_ptr));
    begin_thread(&s_singleton.m_thread_stack.top());
  }

  /// Demarcate the start of a new child thread
  static void begin_thread(Thread* child_thread_ptr) {
    Thread& child_thread = *child_thread_ptr;
    if (child_thread.parent_thread_ptr()) {
      Thread& parent_thread = *child_thread.parent_thread_ptr();
      const std::shared_ptr<SendEvent> send_event_ptr(new SendEvent(
        parent_thread.thread_id(), parent_thread.path_condition_ptr()));
      slice().append(parent_thread.thread_id(), send_event_ptr);

      std::unique_ptr<ReceiveEvent> receive_event_ptr(new ReceiveEvent(
        child_thread.thread_id(), send_event_ptr->zone(),
        child_thread.path_condition_ptr()));

      slice().append(child_thread.thread_id(), std::move(receive_event_ptr));
    }
    set_current_thread_ptr(child_thread_ptr);
  }

  /// Stop the recording of the current thread of execution 

  /// \returns event that demarcates the end of the recorded thread
  static std::shared_ptr<SendEvent> end_thread() {
    const std::shared_ptr<SendEvent> send_event_ptr(new SendEvent(
      ThisThread::thread_id(), ThisThread::path_condition_ptr()));

    slice().append(ThisThread::thread_id(), send_event_ptr);
    set_current_thread_ptr(current_thread().parent_thread_ptr());

    if (!s_singleton.m_thread_stack.empty()) {
      s_singleton.m_thread_stack.pop();
    }

    return send_event_ptr;
  }

  /// Start recording the main thread

  /// \pre: There are no unfinished thread recordings in progress
  /// \remark The precondition is ensured by Threads::reset()
  static void begin_main_thread() {
    assert(s_singleton.m_thread_stack.empty());
    begin_thread();
  }

  /// Calls end_thread(Z3C0&) and then encode(Z3C0&)

  /// \pre begin_main_thread() must have been called previously
  ///
  /// \returns is there at least one error condition to check?
  static bool end_main_thread(Z3C0& z3) {
    assert(s_singleton.m_thread_stack.size() == 1);
    end_thread();
    return encode(z3);
  }

  /// Call before entering the `do { ... } while(slicer.next_slice())` loop
  static void begin_slice_loop() {
    s_singleton.m_slice.save_slice();
  }

  /// Symbolically encodes all sliced memory accesses between threads

  /// \returns is there at least one error condition to check?
  static bool encode(Z3C0& z3) {
    ZoneRelation<Event> zone_relation;
    const Z3ValueEncoderC0 value_encoder;
    const Z3OrderEncoderC0 order_encoder;

    const z3::symbol epoch_symbol(z3.context.str_symbol("epoch"));
    for (Slice::EventPtrsMap::const_reference event_ptrs_map_value :
      /* per-thread list of event pointers */ slice().event_ptrs_map()) {

      z3::expr clock(z3.context.constant(epoch_symbol, z3.clock_sort()));
      const EventPtrs& event_ptrs = event_ptrs_map_value.second;
      for (EventPtrs::const_reference event_ptr : event_ptrs) {
        if (event_ptr->is_write()) {
          const z3::expr equality_expr(event_ptr->encode_eq(value_encoder, z3));
          z3.solver.add(equality_expr);
        }

        if (!event_ptr->zone().is_bottom()) {
          zone_relation.relate(event_ptr);

          z3::expr next_clock(z3.clock(*event_ptr));
          z3.solver.add(z3.happens_before(clock, next_clock));
          clock = next_clock;
        }
      }
    }

    z3.solver.add(order_encoder.rf_enc(zone_relation, z3));
    z3.solver.add(order_encoder.fr_enc(zone_relation, z3));
    z3.solver.add(order_encoder.ws_enc(zone_relation, z3));

    bool has_error_conditions = !s_singleton.m_error_exprs.empty();
    if (has_error_conditions) {
      z3::expr some_error_expr(z3.context.bool_val(false));
      for (const z3::expr& error_expr : s_singleton.m_error_exprs) {
        some_error_expr = some_error_expr or error_expr;
      }
      z3.solver.add(some_error_expr);

      s_singleton.m_error_exprs.clear();
    }
    return has_error_conditions;
  }

  static void join(const std::shared_ptr<SendEvent>& send_event_ptr) {
    std::unique_ptr<ReceiveEvent> receive_event_ptr(new ReceiveEvent(
      ThisThread::thread_id(), send_event_ptr->zone(),
      ThisThread::path_condition_ptr()));

    slice().append(ThisThread::thread_id(), std::move(receive_event_ptr));
  }

  /// \internal Assert given condition in the SAT solver outside of any thread

  /// \pre: All read events in the condition must only access thread-local
  ///       memory (i.e. Zone::is_bottom() must be true)
  ///
  /// \warning Path conditions are ignored and an unsatisfiable error
  ///          condition renders any others unsatisfiable as well
  static void internal_error(std::unique_ptr<ReadInstr<bool>> condition_ptr, Z3C0& z3) {
    const Z3ValueEncoderC0 value_encoder;
    const z3::expr condition_expr(value_encoder.encode_eq(
      std::move(condition_ptr), z3));

    z3.solver.add(condition_expr);
  }

  /// Assert condition with the current thread's path condition as antecedent
  static void expect(std::unique_ptr<ReadInstr<bool>> condition_ptr, Z3C0& z3) {
    slice().append_all(ThisThread::thread_id(), *condition_ptr);

    const Z3ValueEncoderC0 value_encoder;
    const z3::expr condition_expr(value_encoder.encode_eq(
      std::move(condition_ptr), z3));

    const std::shared_ptr<ReadInstr<bool>> path_condition_ptr(
      ThisThread::path_condition_ptr());
    if (path_condition_ptr) {
      const Z3ReadEncoderC0 read_encoder;
      z3.solver.add(implies(path_condition_ptr->encode(read_encoder, z3),
        condition_expr));
    } else {
      z3.solver.add(condition_expr);
    }
  }

  /// Assert condition in the SAT solver and record the condition's read events

  /// \remark The logical disjunction of all given error conditions allows
  ///         multiple of them to be checked simultaneously by the SAT solver
  static void error(std::unique_ptr<ReadInstr<bool>> condition_ptr, Z3C0& z3) {
    slice().append_all(ThisThread::thread_id(), *condition_ptr);

    const Z3ValueEncoderC0 value_encoder;
    const z3::expr error_condition_expr(value_encoder.encode_eq(
      std::move(condition_ptr), z3));

    const std::shared_ptr<ReadInstr<bool>> path_condition_ptr(
      ThisThread::path_condition_ptr());
    if (path_condition_ptr) {
      const Z3ReadEncoderC0 read_encoder;
      s_singleton.m_error_exprs.push_front(error_condition_expr and
        path_condition_ptr->encode(read_encoder, z3));
    } else {
      s_singleton.m_error_exprs.push_front(error_condition_expr);
    }
  }
};

template<typename Function, typename... Args>
Thread::Thread(Function&& f, Args&&... args) :
  m_thread_id(s_next_thread_id++),
  m_parent_thread_ptr(&Threads::current_thread()),
  m_send_event_ptr(nullptr),
  m_condition_ptrs_size(0),
  m_condition_ptrs() {

  Threads::begin_thread(this);
  f(args...);
  m_send_event_ptr = Threads::end_thread();
}

template<typename T>
std::shared_ptr<DirectWriteEvent<T>> Thread::instr(const Zone& zone,
  std::unique_ptr<ReadInstr<T>> instr_ptr) {

  Threads::slice().append_all<T>(m_thread_id, *instr_ptr);

  std::shared_ptr<DirectWriteEvent<T>> write_event_ptr(new DirectWriteEvent<T>(
    m_thread_id, zone, std::move(instr_ptr), path_condition_ptr()));

  Threads::slice().append(m_thread_id, write_event_ptr);
  return write_event_ptr;
}

template<typename T, typename U, size_t N>
std::shared_ptr<IndirectWriteEvent<T, U, N>> Thread::instr(const Zone& zone,
  std::unique_ptr<DerefReadInstr<T[N], U>> deref_instr_ptr,
  std::unique_ptr<ReadInstr<T>> instr_ptr) {

  Threads::slice().append_all<T>(m_thread_id, *instr_ptr);
  Threads::slice().append_all<T>(m_thread_id, *deref_instr_ptr);

  std::shared_ptr<IndirectWriteEvent<T, U, N>> write_event_ptr(
    new IndirectWriteEvent<T, U, N>(m_thread_id, zone,
      std::move(deref_instr_ptr), std::move(instr_ptr),
        path_condition_ptr()));

  Threads::slice().append(m_thread_id, write_event_ptr);
  return write_event_ptr;
}

namespace ThisThread {
  /// Records a direct memory write event

  /// Records direct write event and the read events that determine its value.
  template<typename T>
  std::shared_ptr<DirectWriteEvent<T>> instr(const Zone& zone,
    std::unique_ptr<ReadInstr<T>> instr_ptr) {

    return Threads::current_thread().instr(zone, std::move(instr_ptr));
  }

  /// Records an indirect memory write event

  /// Records indirect write event and the read events that determine its value.
  template<typename T, typename U, size_t N>
  std::shared_ptr<IndirectWriteEvent<T, U, N>> instr(const Zone& zone,
    std::unique_ptr<DerefReadInstr<T[N], U>> deref_instr_ptr,
    std::unique_ptr<ReadInstr<T>> instr_ptr) {

    return Threads::current_thread().instr(zone, std::move(deref_instr_ptr),
      std::move(instr_ptr));
  }
}

}

#endif
