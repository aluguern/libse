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

  const std::shared_ptr<ReadInstr<bool>> path_condition_ptr() {
    return Threads::current_thread().path_condition_ptr();
  }

  void begin_block(const std::shared_ptr<ReadInstr<bool>>& condition_ptr) {
    Threads::current_thread().begin_block(condition_ptr);
  }

  void end_block() {
    Threads::current_thread().end_block();
  }
};

bool Thread::encode() {
  return Threads::encode(s_z3);
}

/// A satisfiable condition exposes a (concurrency) bug
void Thread::error(std::unique_ptr<ReadInstr<bool>> condition_ptr) {
  Threads::error(std::move(condition_ptr), s_z3);
}

void Thread::begin_block(const std::shared_ptr<ReadInstr<bool>>& condition_ptr) {
  assert(nullptr != condition_ptr);

  Threads::slice_append_all(m_thread_id, *condition_ptr);

  m_condition_ptrs_size++;
  m_condition_ptrs.push_front(condition_ptr);

  if (1 < m_condition_ptrs_size) {
    std::unique_ptr<ReadInstr<bool>> path_condition_ptr(
      new NaryReadInstr<LAND, bool>(m_condition_ptrs, m_condition_ptrs_size));
    m_path_condition_ptr_cache.push(std::move(path_condition_ptr));
  }
}

void Thread::end_block() {
  assert(0 < m_condition_ptrs_size);

  m_condition_ptrs_size--;
  m_condition_ptrs.pop_front();

  if (!m_path_condition_ptr_cache.empty() &&
    m_condition_ptrs_size == m_path_condition_ptr_cache.size()) {

    assert(0 < m_condition_ptrs_size);
    m_path_condition_ptr_cache.pop();
  }
}

std::shared_ptr<ReadInstr<bool>> Thread::path_condition_ptr() {
  if (m_condition_ptrs_size == 0) {
    return s_true_condition_ptr;
  } else if (m_condition_ptrs_size == 1) {
    return m_condition_ptrs.front();
  }

  // path conditions at depth higher than 1 are cached by Thread::begin_block()
  assert(0 < m_condition_ptrs_size);
  assert(m_path_condition_ptr_cache.size() == (m_condition_ptrs_size - 1));

  return m_path_condition_ptr_cache.top();
}

void Thread::join() noexcept {
  Threads::join(m_send_event_ptr);
}

}
