// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_RECORDER_H_
#define LIBSE_CONCURRENT_RECORDER_H_

#include <memory>
#include <stack>
#include <forward_list>

#include "concurrent/event.h"
#include "concurrent/instr.h"

namespace se {

class PathCondition {
private:
  static const std::shared_ptr<ReadInstr<bool>> s_true_condition;

  std::stack<std::shared_ptr<ReadInstr<bool>>> m_conditions;

public:
  void push(std::unique_ptr<ReadInstr<bool>> condition) {
    m_conditions.push(std::move(condition));
  }

  void pop() { m_conditions.pop(); }

  std::shared_ptr<ReadInstr<bool>> top() const {
    if (m_conditions.empty()) {
      return s_true_condition;
    }

    return m_conditions.top();
  }
};

/// Records events and path constraints on a per-thread basis
class Recorder {
private:
  const unsigned m_thread_id;

  PathCondition m_path_condition;
  std::forward_list<std::shared_ptr<Event>> m_event_ptrs;

public:
  Recorder(unsigned thread_id) : m_thread_id(thread_id),
    m_path_condition(), m_event_ptrs() {}

  unsigned thread_id() const { return m_thread_id; }
  PathCondition& path_condition() { return m_path_condition; }

  std::forward_list<std::shared_ptr<Event>>& event_ptrs() {
    return m_event_ptrs;
  }

  /// Records a direct memory write event

  /// \returns a shared pointer to the newly recorded WriteEvent<T>
  template<typename T>
  std::shared_ptr<WriteEvent<T>> instr(const MemoryAddr& addr,
    std::unique_ptr<ReadInstr<T>> instr_ptr) {

    std::unique_ptr<ReadInstr<T>> null_ptr;
    return instr(addr, std::move(null_ptr), std::move(instr_ptr));
  }

  /// Records a memory write event

  /// \returns a shared pointer to the newly recorded WriteEvent<T>
  template<typename T>
  std::shared_ptr<WriteEvent<T>> instr(const MemoryAddr& addr,
    std::unique_ptr<ReadInstr<T>> deref_instr_ptr,
    std::unique_ptr<ReadInstr<T>> instr_ptr) {

    // Extract from ReadInstr<T> pointer all pointers to ReadEvent<T> objects
    std::forward_list<std::shared_ptr<Event>> read_event_ptrs;
    instr_ptr->filter(read_event_ptrs);

    // Append these pointers to the per-thread event list
     m_event_ptrs.insert_after(m_event_ptrs.cbefore_begin(),
       /* range */ read_event_ptrs.cbegin(), read_event_ptrs.cend());

    bool is_direct = !static_cast<bool>(deref_instr_ptr);

    // Append new write event that writes a memory location
    std::shared_ptr<WriteEvent<T>> write_event_ptr(new WriteEvent<T>(
      m_thread_id, addr, std::move(deref_instr_ptr), std::move(instr_ptr),
      path_condition().top()));

    assert(is_direct == write_event_ptr->is_direct());

    m_event_ptrs.push_front(write_event_ptr);

    return write_event_ptr;
  }
};

/// Static object which can record events and path conditions
extern std::shared_ptr<Recorder> recorder_ptr();
extern void set_recorder(const std::shared_ptr<Recorder>& recorder_ptr);

}

#endif
