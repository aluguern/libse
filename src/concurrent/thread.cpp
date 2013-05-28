// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "concurrent/thread.h"

namespace se {

Threads Threads::s_singleton;
Z3C0 Thread::s_z3;
ThreadId Thread::s_next_thread_id(0);
const std::shared_ptr<ReadInstr<bool>> Thread::s_true_condition_ptr;

namespace ThisThread {
  ThreadId thread_id() {
    return Threads::current_thread().thread_id();
  }

  const Thread* parent_thread_ptr() {
    return Threads::current_thread().parent_thread_ptr();
  }

  std::shared_ptr<Block> most_outer_block_ptr() {
    return Threads::slice_most_outer_block_ptr(thread_id());
  }

  const std::shared_ptr<ReadInstr<bool>> path_condition_ptr() {
    return Threads::current_thread().path_condition_ptr();
  }

  void begin_then(std::shared_ptr<ReadInstr<bool>> condition_ptr) {
    Threads::current_thread().begin_then(condition_ptr);
  }

  void begin_else() {
    Threads::current_thread().begin_else();
  }

  void end_branch() {
    Threads::current_thread().end_branch();
  }
};

bool Thread::encode() {
  return Threads::encode(s_z3);
}

/// A satisfiable condition exposes a (concurrency) bug
void Thread::error(std::unique_ptr<ReadInstr<bool>> condition_ptr) {
  Threads::error(std::move(condition_ptr), s_z3);
}

void Thread::begin_then(std::shared_ptr<ReadInstr<bool>> condition_ptr) {
  assert(nullptr != condition_ptr);

  register_condition(condition_ptr);
  Threads::slice_begin_then(m_thread_id, condition_ptr);
}

void Thread::begin_else() {
  const std::shared_ptr<ReadInstr<bool>> negated_condition_ptr =
    Bools::negate(unregister_condition());

  register_condition(negated_condition_ptr);
  Threads::slice_begin_else(m_thread_id);
}

void Thread::end_branch() {
  unregister_condition();
  Threads::slice_end_branch(m_thread_id);
}

std::shared_ptr<ReadInstr<bool>> Thread::path_condition_ptr() {
  if (m_condition_ptrs_size == 0) {
    return s_true_condition_ptr;
  } else if (m_condition_ptrs_size == 1) {
    return m_condition_ptrs.front();
  }

  assert(0 < m_condition_ptrs_size);
  assert(m_path_condition_ptr_cache.size() == (m_condition_ptrs_size - 1));

  return m_path_condition_ptr_cache.top();
}

void Thread::join() noexcept {
  Threads::join(m_send_event_ptr);
}

}
