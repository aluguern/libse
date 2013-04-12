// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_THREAD_H_
#define LIBSE_CONCURRENT_THREAD_H_

#include <stack>

#include "concurrent/tag.h"
#include "concurrent/event.h"
#include "concurrent/encoder_c0.h"
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

  const Z3ValueEncoderC0 m_value_encoder;
  const Z3OrderEncoderC0 m_order_encoder;
  std::stack<Recorder> m_recorder_stack;
  unsigned m_next_thread_id;
  TagRelation<Event> m_tag_relation;

  // Must only be accessed before Z3 solver is deallocated
  std::forward_list<z3::expr> m_error_exprs;

private:
  Threads() : m_value_encoder(), m_order_encoder(), m_next_thread_id(0),
    m_recorder_stack(), m_tag_relation(), m_error_exprs() {

    internal_reset(0, 0);
  }

  // Clears the entire recorder stack and restarts recording the main thread
  void internal_reset(size_t next_event_id, uintptr_t next_tag) {
    Event::reset_id(next_event_id);
    Tag::reset(next_tag);

    while (!m_recorder_stack.empty()) { 
      m_recorder_stack.pop();
    }

    m_tag_relation.clear();
    assert(m_error_exprs.empty());
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
      if (event_ptr->tag().is_bottom()) { continue; }
      m_tag_relation.relate(event_ptr);
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
  static void reset(size_t next_event_id = 0, uintptr_t next_tag = 0) {
    return s_singleton.internal_reset(next_event_id, next_tag);
  }

  /// Start recording a new thread of execution
  static void begin_thread() {
    Recorder child_recorder(s_singleton.m_next_thread_id++);

    if (!s_singleton.m_recorder_stack.empty()) {
      Recorder& parent_recorder = s_singleton.current_recorder();
      const std::shared_ptr<SendEvent> send_event_ptr(new SendEvent(
        parent_recorder.thread_id(), parent_recorder.block_condition_ptr()));
      parent_recorder.insert_event_ptr(send_event_ptr);

      std::unique_ptr<ReceiveEvent> receive_event_ptr(new ReceiveEvent(
        child_recorder.thread_id(), send_event_ptr->tag(),
        child_recorder.block_condition_ptr()));

      child_recorder.insert_event_ptr(std::move(receive_event_ptr));
    }

    s_singleton.m_recorder_stack.push(std::move(child_recorder));
  }

  /// Encode the concurrent SSA and control structure of the current thread

  /// \returns event that demarcates the end of the recorded thread
  static std::shared_ptr<SendEvent> end_thread(Z3& z3) {
    Recorder& current_recorder = s_singleton.current_recorder();
    const std::shared_ptr<SendEvent> send_event_ptr(new SendEvent(
      current_recorder.thread_id(), current_recorder.block_condition_ptr()));

    current_recorder.insert_event_ptr(send_event_ptr);

    const std::shared_ptr<Block>& most_outer_block_ptr(
      current_recorder.most_outer_block_ptr());

    s_singleton.internal_encode_events(most_outer_block_ptr, z3);
    s_singleton.internal_relate_events(most_outer_block_ptr);
    s_singleton.m_order_encoder.encode_spo(most_outer_block_ptr, z3);

    s_singleton.m_recorder_stack.pop();

    return send_event_ptr;
  }

  /// Start recording the main thread

  /// \pre: There are no unfinished thread recordings in progress
  /// \remark The precondition is ensured by Threads::reset()
  static void begin_main_thread() {
    assert(s_singleton.m_recorder_stack.empty());
    begin_thread();
  }

  /// Calls end_thread(Z3&) and then encodes all memory accesses between threads

  /// \warning Must always be called after begin_main_thread()
  static void end_main_thread(Z3& z3) {
    assert(s_singleton.m_recorder_stack.size() == 1);
    end_thread(z3);

    const TagRelation<Event>& rel = s_singleton.m_tag_relation;
    const Z3OrderEncoderC0& order_encoder = s_singleton.m_order_encoder;

    z3.solver.add(order_encoder.rfe_encode(rel, z3));
    z3.solver.add(order_encoder.fr_encode(rel, z3));
    z3.solver.add(order_encoder.ws_encode(rel, z3));

    if (!s_singleton.m_error_exprs.empty()) {
      z3::expr some_error_expr(z3.context.bool_val(false));
      for (const z3::expr& error_expr : s_singleton.m_error_exprs) {
        some_error_expr = some_error_expr or error_expr;
      }
      z3.solver.add(some_error_expr);

      s_singleton.m_error_exprs.clear();
    }
  }

  static void join(const std::shared_ptr<SendEvent>& send_event_ptr) {
    Recorder& current_recorder = s_singleton.current_recorder();
    std::unique_ptr<ReceiveEvent> receive_event_ptr(new ReceiveEvent(
      current_recorder.thread_id(), send_event_ptr->tag(),
      current_recorder.block_condition_ptr()));

    current_recorder.insert_event_ptr(std::move(receive_event_ptr));
  }

  /// \internal Assert given condition in the SAT solver outside of any thread

  /// \pre: All read events in the condition must only access thread-local
  ///       memory (i.e. Tag::is_bottom() must be true)
  ///
  /// \warning Block conditions are ignored and an unsatisfiable error
  ///          condition renders any others unsatisfiable as well
  static void internal_error(std::unique_ptr<ReadInstr<bool>> condition_ptr, Z3& z3) {
    const z3::expr condition_expr(s_singleton.m_value_encoder.encode_eq(
      std::move(condition_ptr), z3));

    z3.solver.add(condition_expr);
  }

  /// Assert condition with the current block condition as antecedent
  static void expect(std::unique_ptr<ReadInstr<bool>> condition_ptr, Z3& z3) {
    ThisThread::recorder().insert_all(*condition_ptr);

    const z3::expr condition_expr(s_singleton.m_value_encoder.encode_eq(
      std::move(condition_ptr), z3));

    const std::shared_ptr<ReadInstr<bool>> block_condition_ptr(
      ThisThread::recorder().block_condition_ptr());
    if (block_condition_ptr) {
      const Z3ReadEncoderC0 read_encoder;
      z3.solver.add(implies(block_condition_ptr->encode(read_encoder, z3),
        condition_expr));
    } else {
      z3.solver.add(condition_expr);
    }
  }

  /// Assert condition in the SAT solver and record the condition's read events

  /// \remark The logical disjunction of all given error conditions allows
  ///         multiple of them to be checked simultaneously by the SAT solver
  static void error(std::unique_ptr<ReadInstr<bool>> condition_ptr, Z3& z3) {
    ThisThread::recorder().insert_all(*condition_ptr);

    const z3::expr error_condition_expr(s_singleton.m_value_encoder.encode_eq(
      std::move(condition_ptr), z3));

    const std::shared_ptr<ReadInstr<bool>> block_condition_ptr(
      ThisThread::recorder().block_condition_ptr());
    if (block_condition_ptr) {
      const Z3ReadEncoderC0 read_encoder;
      s_singleton.m_error_exprs.push_front(error_condition_expr and
        block_condition_ptr->encode(read_encoder, z3));
    } else {
      s_singleton.m_error_exprs.push_front(error_condition_expr);
    }
  }
};

/// Symbolic thread for the analysis of concurrent C++ code
class Thread {
private:
  static Z3 s_z3;

  // Constant and never null once the constructor body has been executed
  std::shared_ptr<SendEvent> m_send_event_ptr;

public:
  static Z3& z3() { return s_z3; }

  /// Terminate the recording of the main thread
  static void end_main_thread() { Threads::end_main_thread(s_z3); }

  /// Give a property that exposes a bug if it is satisfiable
  static void error(std::unique_ptr<ReadInstr<bool>> condition_ptr) {
    Threads::error(std::move(condition_ptr), s_z3);
  }

  /// Symbolically spawn `f(args...)` as a new thread of execution

  /// \param f non-member function to be executed as a new symbolic thread
  /// \param args arguments \ref std::forward "forwarded" to `f`
  ///
  /// The return value of `f` is always ignored.
  template<typename Function, typename... Args>
  explicit Thread(Function&& f, Args&&... args) {
    Threads::begin_thread();
    f(args...);
    m_send_event_ptr = Threads::end_thread(s_z3);
  }

  void join() noexcept { Threads::join(m_send_event_ptr); }
};

}

#endif
