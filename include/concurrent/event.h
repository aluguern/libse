// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_EVENT_H_
#define LIBSE_CONCURRENT_EVENT_H_

#include <cassert>
#include <cstddef>
#include <memory>

#include "core/type.h"

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
/// a \ref Event::event_id() "unique identifier".
///
/// An event guarded by a non-null \ref Event::condition_ptr() "condition"
/// is said to be conditional; otherwise, it is said to be unconditional.
class Event {
private:
  // On 32-bit architectures, the legal range of identifiers is 0 to 2^15-1.
  static size_t s_next_id;

  const size_t m_event_id;
  const unsigned m_thread_id;
  const bool m_is_read;
  const Type* const m_type_ptr;
  const MemoryAddr m_addr;
  const std::shared_ptr<ReadInstr<bool>> m_condition_ptr;

  // Even positive integers (m = 2k for k in 0 to 2^15-1) are for read
  // events. In turn, odd positive integers (n = 2k + 1 for k in 0 to 2^15-1)
  // are write events. That is, the maximal read event identifier is 2^30-2
  // and the maximal write event identifier is 2^30-1. These upper limits
  // stem from Z3 which aligns char pointers for symbol names by 4 bytes on
  // 32-bit architectures.
  static size_t next_id(bool is_read) {
    const size_t even = 2 * s_next_id++;
    return is_read ? even : even + 1;
  }

protected:

  /// Create a unique read or write event

  /// \param addr - memory address read or written by the event
  /// \param thread_id - unique thread identifier
  /// \param is_read - does the event cause memory to be read?
  /// \param type_ptr - pointer to statically allocated memory, never null
  /// \param condition_ptr - necessary condition for the event to occur
  ///
  /// The type_ptr describes the event in terms of its memory characteristics
  /// such as how many bytes are read or written.
  Event(unsigned thread_id, const MemoryAddr& addr, bool is_read,
    const Type* const type_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    m_event_id(next_id(is_read)), m_addr(addr), m_thread_id(thread_id),
    m_is_read(is_read), m_type_ptr(type_ptr), m_condition_ptr(condition_ptr) {

    assert(type_ptr != nullptr);
  }

public:
  static void reset_id(size_t id = 0) { s_next_id = id; }

  virtual ~Event() {}

  size_t event_id() const { return m_event_id; }
  unsigned thread_id() const { return m_thread_id; }
  const MemoryAddr& addr() const { return m_addr; }
  bool is_read() const { return m_is_read; }
  bool is_write() const { return !m_is_read; }
  const Type& type() const { return *m_type_ptr; }

  /// Condition that guards the event

  /// \returns nullptr if the event is unconditional
  const std::shared_ptr<ReadInstr<bool>> condition_ptr() const {
    return m_condition_ptr;
  }

  bool operator==(const Event& other) const {
    return m_event_id == other.m_event_id;
  }
};

/// Event that writes `sizeof(T)` bytes to memory
template<typename T>
class WriteEvent : public Event {
private:
  // Never null
  const std::unique_ptr<ReadInstr<T>> m_instr_ptr;

protected:
  WriteEvent(unsigned thread_id, const MemoryAddr& addr,
    std::unique_ptr<ReadInstr<T>> instr_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(thread_id, addr, false, &TypeInfo<T>::s_type, condition_ptr),
    m_instr_ptr(std::move(instr_ptr)) {

    assert(nullptr != m_instr_ptr);
  }

public:
  virtual ~WriteEvent() {}

  const ReadInstr<T>& instr_ref() const { return *m_instr_ptr; }
};

/// Memory write without another address load required
template<typename T>
class DirectWriteEvent : public WriteEvent<T> {
public:
  DirectWriteEvent(unsigned thread_id, const MemoryAddr& addr,
    std::unique_ptr<ReadInstr<T>> instr_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    WriteEvent<T>(thread_id, addr, std::move(instr_ptr), condition_ptr) {}

  ~DirectWriteEvent() {}
};

template<typename T, typename U> class DerefReadInstr;

/// Memory write that depends on an address load
template<typename T, typename U, size_t N>
class IndirectWriteEvent : public WriteEvent<T> {
private:
  // Never null
  const std::unique_ptr<DerefReadInstr<T[N], U>> m_deref_instr_ptr;

public:
  IndirectWriteEvent(unsigned thread_id, const MemoryAddr& addr,
    std::unique_ptr<DerefReadInstr<T[N], U>> deref_instr_ptr,
    std::unique_ptr<ReadInstr<T>> instr_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    WriteEvent<T>(thread_id, addr, std::move(instr_ptr), condition_ptr), 
    m_deref_instr_ptr(std::move(deref_instr_ptr)) {

    assert(nullptr != m_deref_instr_ptr);
  }

  ~IndirectWriteEvent() {}

  const DerefReadInstr<T[N], U>& deref_instr_ref() const {
    return *m_deref_instr_ptr;
  }
};

/// Event that reads `sizeof(T)` bytes from memory
template<typename T>
class ReadEvent : public Event {
public:
  ReadEvent(unsigned thread_id, const MemoryAddr& addr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(thread_id, addr, true, &TypeInfo<T>::s_type, condition_ptr) {}

  ~ReadEvent() {}
};

}

#endif
