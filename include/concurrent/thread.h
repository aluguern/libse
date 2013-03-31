// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_THREAD_H_
#define LIBSE_CONCURRENT_THREAD_H_

#include <stack>

#include "concurrent/event.h"
#include "concurrent/memory.h"
#include "concurrent/encoder.h"
#include "concurrent/recorder.h"

namespace se {

namespace ThisThread {
  /// Static object that can record events and path conditions
  Recorder& recorder();

  /// Current thread identifier
  unsigned thread_id();
};

/// \internal Thread helper singleton
class Threads {
private:
  friend Recorder& ThisThread::recorder();
  friend unsigned ThisThread::thread_id();
  static Threads s_singleton;

  const Z3ValueEncoder m_value_encoder;
  const Z3OrderEncoder m_order_encoder;
  std::stack<Recorder> m_recorder_stack;
  unsigned m_next_thread_id;
  MemoryAddrRelation<Event> m_memory_addr_relation;

private:
  Threads() : m_value_encoder(), m_order_encoder(), m_next_thread_id(0),
    m_recorder_stack(), m_memory_addr_relation() {

    internal_reset(0, 0);
  }

  // Clears the entire recorder stack and restarts recording the main thread
  void internal_reset(size_t next_event_id, uintptr_t next_addr) {
    Event::reset_id(next_event_id);
    MemoryAddr::reset(next_addr);

    while (!m_recorder_stack.empty()) { 
      m_recorder_stack.pop();
    }

    m_memory_addr_relation.clear();
  }

  // Encodes all write events reachable from the given block
  void internal_encode_events(const std::shared_ptr<Block>& block_ptr, Z3& z3) {
    for (const std::shared_ptr<Event>& event_ptr : block_ptr->body()) {
      if (event_ptr->is_write()) {
        const z3::expr equality_expr(event_ptr->encode_eq(m_value_encoder, z3));
        z3.solver.add(equality_expr);
      }
    }

    for (const std::shared_ptr<Block>& inner_block_ptr :
      block_ptr->inner_block_ptrs()) {

      internal_encode_events(inner_block_ptr, z3);
    }

    if (block_ptr->else_block_ptr()) {
      internal_encode_events(block_ptr->else_block_ptr(), z3);
    }
  }

  // Relates all events reachable from the given block
  void internal_relate_events(const std::shared_ptr<Block>& block_ptr) {
    for (const std::shared_ptr<Event>& event_ptr : block_ptr->body()) {
      m_memory_addr_relation.relate(event_ptr);
    }

    for (const std::shared_ptr<Block>& inner_block_ptr :
      block_ptr->inner_block_ptrs()) {

      internal_relate_events(inner_block_ptr);
    }

    if (block_ptr->else_block_ptr()) {
      internal_relate_events(block_ptr->else_block_ptr());
    }
  }

protected:
  static unsigned current_thread_id() {
    return s_singleton.current_recorder().thread_id();
  }

  /// \pre: Threads::begin_thread() must have been called at least once
  static Recorder& current_recorder() {
    assert(!s_singleton.m_recorder_stack.empty());
    return s_singleton.m_recorder_stack.top();
  }

public:
  /// Erase any previous thread recordings
  static void reset(size_t next_event_id = 0, uintptr_t next_addr = 0) {
    return s_singleton.internal_reset(next_event_id, next_addr);
  }

  /// Start recording a new thread of execution
  static void begin_thread() {
    s_singleton.m_recorder_stack.push(Recorder(s_singleton.m_next_thread_id++));
  }

  /// Encode the concurrent SSA and control structure of the current thread
  static void end_thread(Z3& z3) {
    const std::shared_ptr<Block>& most_outer_block_ptr(
      current_recorder().most_outer_block_ptr());

    s_singleton.internal_encode_events(most_outer_block_ptr, z3);
    s_singleton.internal_relate_events(most_outer_block_ptr);
    s_singleton.m_order_encoder.encode_spo(most_outer_block_ptr, z3);

    s_singleton.m_recorder_stack.pop();
  }

  /// Start recording the main thread

  /// \pre: There are no unfinished thread recordings in progress
  /// \remark The precondition is ensured by Threads::reset()
  static void begin_main_thread() {
    assert(s_singleton.m_recorder_stack.empty());
    begin_thread();
  }

  /// Calls end_thread(Z3&) and then encodes all memory accesses between threads
  static void end_main_thread(Z3& z3) {
    end_thread(z3);

    const MemoryAddrRelation<Event>& rel = s_singleton.m_memory_addr_relation;
    const Z3OrderEncoder& order_encoder = s_singleton.m_order_encoder;

    z3.solver.add(order_encoder.rfe_encode(rel, z3));
    z3.solver.add(order_encoder.fr_encode(rel, z3));
    z3.solver.add(order_encoder.ws_encode(rel, z3));
  }

  /// Add the negation of the given condition as a conjecture to the SAT solver
  static void expect(std::unique_ptr<ReadInstr<bool>> condition_ptr, Z3& z3) {
    std::unique_ptr<ReadInstr<bool>> negation_ptr(new UnaryReadInstr<NOT, bool>(
      std::move(condition_ptr)));

    const z3::expr boolean_expr(s_singleton.m_value_encoder.encode_eq(
      std::move(negation_ptr), z3));

    z3.solver.add(boolean_expr);
  }
};

/// Symbolic thread for the analysis of concurrent C++ code
class Thread {
private:
  static Z3 s_z3;

public:

  /// Symbolically spawn `f(args...)` as a new thread of execution

  /// \param f non-member function to be executed as a new symbolic thread
  /// \param args arguments \ref std::forward "forwarded" to `f`
  ///
  /// The return value of `f` is always ignored.
  template<typename Function, typename... Args>
  explicit Thread(Function&& f, Args&&... args) {
    Threads::begin_thread();
    f(args...);
    Threads::end_thread(s_z3);
  }

  void join() noexcept {

  }
};

}

#endif
