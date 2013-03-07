// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_EVENT_H_
#define LIBSE_CONCURRENT_EVENT_H_

#include <cassert>
#include <cstddef>
#include <memory>

#include "concurrent/memory.h"

namespace se {

template<typename T>
class ReadInstr;

/// Untyped read or write event

/// An Event object can only be instantiated through a subclass. Usually,
/// these objects should be made accessible through a shared smart pointer
/// such as std::shared_ptr because during the analysis events often need
/// to be put in \ref Relation "relation" to each other.
///
/// An axiom is that two events `e` and `e'` are equal if and only if
/// both their heap references are identical, i.e. `&e == &e'`. To enforce
/// this equality axiom for subclasses as well, each Event object has
/// a \ref Event::id() "unique identifier".
///
/// An event guarded by a non-null \ref Event::condition_ptr() "condition"
/// is said to be conditional; otherwise, it is said to be unconditional.
class Event {
private:
  // On 32-bit architectures, the legal range of identifiers is 0 to 2^15-1.
  static size_t s_next_id;

  const size_t m_id;
  const bool m_is_read;
  const MemoryAddr m_addr;
  const std::shared_ptr<ReadInstr<bool>> m_condition_ptr;

protected:
  Event(const MemoryAddr& addr, bool is_read,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    m_id(s_next_id++), m_addr(addr), m_is_read(is_read),
    m_condition_ptr(condition_ptr) {}

public:
  static void reset_id(size_t id = 0) { s_next_id = id; }

  virtual ~Event() {}

  size_t id() const { return m_id; }
  const MemoryAddr& addr() const { return m_addr; }
  bool is_read() const { return m_is_read; }
  bool is_write() const { return !m_is_read; }

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
  // Never null
  const std::unique_ptr<ReadInstr<T>> m_instr_ptr;

public:
  WriteEvent(const MemoryAddr& addr,
    std::unique_ptr<ReadInstr<T>> instr_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(addr, false, condition_ptr), m_instr_ptr(std::move(instr_ptr)) {

    assert(nullptr != m_instr_ptr);
  }

  ~WriteEvent() {}

  const ReadInstr<T>& instr_ref() const { return *m_instr_ptr; }
};

template<typename T>
class ReadEvent : public Event {
public:
  ReadEvent(const MemoryAddr& addr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(addr, true, condition_ptr) {}

  ~ReadEvent() {}
};

}

#endif
