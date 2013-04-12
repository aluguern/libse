// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_EVENT_H_
#define LIBSE_CONCURRENT_EVENT_H_

#include <cassert>
#include <cstddef>
#include <memory>

#include "core/type.h"

#include "concurrent/tag.h"

namespace z3 { class expr; }

namespace se {

template<typename T>
class ReadInstr;

class Z3;
class Z3ValueEncoderC0;

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
  const Tag m_tag;
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

  /// \param tag - label to link up events
  /// \param thread_id - unique thread identifier
  /// \param is_read - does the event cause memory to be read?
  /// \param type_ptr - pointer to statically allocated memory, never null
  /// \param condition_ptr - necessary condition for the event to occur
  ///
  /// The type_ptr describes the event in terms of its memory characteristics
  /// such as how many bytes are read or written.
  Event(unsigned thread_id, const Tag& tag, bool is_read,
    const Type* const type_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    m_event_id(next_id(is_read)), m_tag(tag), m_thread_id(thread_id),
    m_is_read(is_read), m_type_ptr(type_ptr), m_condition_ptr(condition_ptr) {

    assert(type_ptr != nullptr);
  }

  /// Create a unique read or write event with a specific identifier

  /// \param event_id - unique event identifier
  /// \param tag - label to link up events
  /// \param thread_id - unique thread identifier
  /// \param is_read - does the event cause memory to be read?
  /// \param type_ptr - pointer to statically allocated memory, never null
  /// \param condition_ptr - necessary condition for the event to occur
  ///
  /// The type_ptr describes the event in terms of its memory characteristics
  /// such as how many bytes are read or written.
  Event(size_t event_id, unsigned thread_id, const Tag& tag,
    bool is_read, const Type* const type_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    m_event_id(event_id), m_tag(tag), m_thread_id(thread_id),
    m_is_read(is_read), m_type_ptr(type_ptr), m_condition_ptr(condition_ptr) {

    assert(type_ptr != nullptr);
  }

public:
  static void reset_id(size_t id = 0) { s_next_id = id; }

  virtual ~Event() {}

  size_t event_id() const { return m_event_id; }
  unsigned thread_id() const { return m_thread_id; }
  const Tag& tag() const { return m_tag; }
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

  virtual z3::expr encode_eq(const Z3ValueEncoderC0& encoder, Z3& helper) const = 0;
  virtual z3::expr constant(Z3& helper) const = 0;
};

#define DECL_VALUE_ENCODER_FN \
  z3::expr encode_eq(const Z3ValueEncoderC0& encoder, Z3& helper) const;

#define DECL_CONSTANT_ENCODER_FN \
  z3::expr constant(Z3& helper) const;

/// Event that writes to memory through a variable of type `T`
template<typename T>
class WriteEvent : public Event {
private:
  // Never null
  const std::unique_ptr<ReadInstr<T>> m_instr_ptr;

protected:
  WriteEvent(unsigned thread_id, const Tag& tag,
    std::unique_ptr<ReadInstr<T>> instr_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(thread_id, tag, false, &TypeInfo<T>::s_type, condition_ptr),
    m_instr_ptr(std::move(instr_ptr)) {

    assert(nullptr != m_instr_ptr);
  }

public:
  virtual ~WriteEvent() {}

  const ReadInstr<T>& instr_ref() const { return *m_instr_ptr; }
};

/// Direct memory write event

/// If T is an array type, a direct write event has the effect of initializing
/// every array element to the expression given by WriteEvent<T>::instr_ref().
template<typename T>
class DirectWriteEvent : public WriteEvent<T> {
public:
  DirectWriteEvent(unsigned thread_id, const Tag& tag,
    std::unique_ptr<ReadInstr<T>> instr_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    WriteEvent<T>(thread_id, tag, std::move(instr_ptr), condition_ptr) {}

  ~DirectWriteEvent() {}

  DECL_VALUE_ENCODER_FN
  DECL_CONSTANT_ENCODER_FN
};

template<typename T, typename U> class DerefReadInstr;

/// Memory write that requires a memory load instruction
template<typename T, typename U, size_t N>
class IndirectWriteEvent : public WriteEvent<T> {
private:
  // Never null
  const std::unique_ptr<DerefReadInstr<T[N], U>> m_deref_instr_ptr;

public:
  IndirectWriteEvent(unsigned thread_id, const Tag& tag,
    std::unique_ptr<DerefReadInstr<T[N], U>> deref_instr_ptr,
    std::unique_ptr<ReadInstr<T>> instr_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    WriteEvent<T>(thread_id, tag, std::move(instr_ptr), condition_ptr), 
    m_deref_instr_ptr(std::move(deref_instr_ptr)) {

    assert(nullptr != m_deref_instr_ptr);
  }

  ~IndirectWriteEvent() {}

  const DerefReadInstr<T[N], U>& deref_instr_ref() const {
    return *m_deref_instr_ptr;
  }

  DECL_VALUE_ENCODER_FN
  DECL_CONSTANT_ENCODER_FN
};

/// Event that reads `sizeof(T)` bytes from memory
template<typename T>
class ReadEvent : public Event {
private:
  template<typename U>
  friend std::unique_ptr<ReadEvent<U>> internal_make_read_event(
    const Tag& tag, size_t event_id);

  ReadEvent(size_t event_id, unsigned thread_id, const Tag& tag,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(event_id, thread_id, tag, true, &TypeInfo<T>::s_type, condition_ptr) {}

public:
  ReadEvent(unsigned thread_id, const Tag& tag,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(thread_id, tag, true, &TypeInfo<T>::s_type, condition_ptr) {}

  ~ReadEvent() {}

  DECL_VALUE_ENCODER_FN
  DECL_CONSTANT_ENCODER_FN
};

/// \internal Event for thread synchronization
class SyncEvent : public Event {
private:
  typedef bool Sync;

  DECL_VALUE_ENCODER_FN
  DECL_CONSTANT_ENCODER_FN

protected:
  SyncEvent(unsigned thread_id, const Tag& tag, bool receive,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(thread_id, tag, receive, &TypeInfo<Sync>::s_type, condition_ptr) {}
};

/// \internal Write event for thread synchronization

/// Synchronization occurs through a unique \ref Event::tag() "tag atom".
class SendEvent : public SyncEvent {
public:
  SendEvent(unsigned thread_id,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    SyncEvent(thread_id, Tag::unique_atom(), false, condition_ptr) {}
};

/// \internal Read event for thread synchronization
class ReceiveEvent : public SyncEvent {
public:
  /// Event that reads from the given tag, preferably SendEvent::tag()
  ReceiveEvent(unsigned thread_id, const Tag& tag,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    SyncEvent(thread_id, tag, true, condition_ptr) {}
};

}

#endif
