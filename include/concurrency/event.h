// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENCY_EVENT_H_
#define LIBSE_CONCURRENCY_EVENT_H_

#include <cassert>
#include <cstddef>
#include <memory>

#include "concurrency/memory.h"

namespace se {

template<typename T>
class ReadInstr;

/// Untyped read or write event

/// An axiom is that two events `e` and `e'` are equal if and only if
/// both their references are identical, i.e. `&e == &e'`. To enforce
/// this equality axiom for subclasses as well, each Event object has
/// a \ref Event::id() "unique identifier".
///
/// An event guarded by a non-null \ref Event::condition_ptr() "condition"
/// is said to be conditional; otherwise, it is said to be unconditional.
class Event {
private:
  static size_t s_next_id;

  const size_t m_id;
  const MemoryAddr m_addr;
  const std::shared_ptr<ReadInstr<bool>> m_condition_ptr;

public:
  static size_t reset_id(size_t id = 0) { s_next_id = id; }

  Event(const MemoryAddr& addr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    m_id(s_next_id++), m_addr(addr), m_condition_ptr(condition_ptr) {}

  virtual ~Event() {}

  size_t id() const { return m_id; }
  const MemoryAddr& addr() const { return m_addr; }

  /// Condition that guards the event

  /// \returns nullptr if the event is unconditional
  const std::shared_ptr<ReadInstr<bool>> condition_ptr() const {
    return m_condition_ptr;
  }

  bool operator==(const Event& other) const { return m_id == other.m_id; }
};

template<typename T>
class WriteEvent : public Event {
private:
  const std::shared_ptr<ReadInstr<T>> m_instr_ptr;

public:
  WriteEvent(const MemoryAddr& addr,
    const std::shared_ptr<ReadInstr<T>>& instr_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr) :
    Event(addr, condition_ptr), m_instr_ptr(instr_ptr) {}

  ~WriteEvent() {}

  const std::shared_ptr<ReadInstr<T>> instr_ptr() const { return m_instr_ptr; }
};

template<typename T>
class ReadEvent : public Event {
public:
  ReadEvent(const MemoryAddr& addr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr) :
    Event(addr, condition_ptr) {}

  ~ReadEvent() {}
};

}

#endif
