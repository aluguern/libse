// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_EVENT_H_
#define LIBSE_CONCURRENT_EVENT_H_

#include <cassert>
#include <cstddef>
#include <memory>

#include "core/type.h"

#include "concurrent/zone.h"

namespace smt
{
  class UnsafeTerm;
}

namespace se {

template<typename T>
class ReadInstr;

class Encoders;
class ValueEncoder;

// On 32-bit architectures, the maximal write event identifier is 2^30-1.
// This upper limit stems from Z3 which aligns char pointers for symbol
// names by 4 bytes .
typedef unsigned EventId;

typedef unsigned ThreadId;

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
  static unsigned s_next_id;

  const EventId m_event_id;
  const ThreadId m_thread_id;
  const bool m_is_read;
  const Type* const m_type_ptr;
  const Zone m_zone;
  const std::shared_ptr<ReadInstr<bool>> m_condition_ptr;

protected:
  /// Create a unique read or write event

  /// \param zone - label to link up events
  /// \param thread_id - unique thread identifier
  /// \param is_read - does the event cause memory to be read?
  /// \param type_ptr - pointer to statically allocated memory, never null
  /// \param condition_ptr - necessary condition for the event to occur
  ///
  /// The type_ptr describes the event in terms of its memory characteristics
  /// such as how many bytes are read or written.
  Event(ThreadId thread_id, const Zone& zone, bool is_read,
    const Type* const type_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    m_event_id(s_next_id++), m_zone(zone), m_thread_id(thread_id),
    m_is_read(is_read), m_type_ptr(type_ptr), m_condition_ptr(condition_ptr) {

    assert(type_ptr != nullptr);
  }

  /// Create a unique read or write event with a specific identifier

  /// \param event_id - unique event identifier
  /// \param zone - label to link up events
  /// \param thread_id - unique thread identifier
  /// \param is_read - does the event cause memory to be read?
  /// \param type_ptr - pointer to statically allocated memory, never null
  /// \param condition_ptr - necessary condition for the event to occur
  ///
  /// The type_ptr describes the event in terms of its memory characteristics
  /// such as how many bytes are read or written.
  Event(EventId event_id, ThreadId thread_id, const Zone& zone,
    bool is_read, const Type* const type_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    m_event_id(event_id), m_zone(zone), m_thread_id(thread_id),
    m_is_read(is_read), m_type_ptr(type_ptr), m_condition_ptr(condition_ptr) {

    assert(type_ptr != nullptr);
  }

public:
  static void reset_id(unsigned id = 0) { s_next_id = id; }

  virtual ~Event() {}

  EventId event_id() const { return m_event_id; }
  ThreadId thread_id() const { return m_thread_id; }
  const Zone& zone() const { return m_zone; }
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

  virtual smt::UnsafeTerm encode_eq(const ValueEncoder& encoder, Encoders& helper) const = 0;
  virtual smt::UnsafeTerm constant(Encoders& helper) const = 0;
};

#define DECL_VALUE_ENCODER_C0_FN \
  smt::UnsafeTerm encode_eq(const ValueEncoder& encoder, Encoders& helper) const;

#define DECL_CONSTANT_ENCODER_C0_FN \
  smt::UnsafeTerm constant(Encoders& helper) const;

/// Event that writes to memory through a variable of type `T`
template<typename T>
class WriteEvent : public Event {
private:
  // Never null
  const std::unique_ptr<ReadInstr<T>> m_instr_ptr;

protected:
  WriteEvent(ThreadId thread_id, const Zone& zone,
    std::unique_ptr<ReadInstr<T>> instr_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(thread_id, zone, false, &TypeInfo<T>::s_type, condition_ptr),
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
  DirectWriteEvent(ThreadId thread_id, const Zone& zone,
    std::unique_ptr<ReadInstr<T>> instr_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    WriteEvent<T>(thread_id, zone, std::move(instr_ptr), condition_ptr) {}

  ~DirectWriteEvent() {}

  DECL_VALUE_ENCODER_C0_FN
  DECL_CONSTANT_ENCODER_C0_FN
};

template<typename T, typename U> class DerefReadInstr;

/// Memory write that requires a memory load instruction
template<typename T, typename U, size_t N>
class IndirectWriteEvent : public WriteEvent<T> {
private:
  // Never null
  const std::unique_ptr<DerefReadInstr<T[N], U>> m_deref_instr_ptr;

public:
  IndirectWriteEvent(ThreadId thread_id, const Zone& zone,
    std::unique_ptr<DerefReadInstr<T[N], U>> deref_instr_ptr,
    std::unique_ptr<ReadInstr<T>> instr_ptr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    WriteEvent<T>(thread_id, zone, std::move(instr_ptr), condition_ptr), 
    m_deref_instr_ptr(std::move(deref_instr_ptr)) {

    assert(nullptr != m_deref_instr_ptr);
  }

  ~IndirectWriteEvent() {}

  const DerefReadInstr<T[N], U>& deref_instr_ref() const {
    return *m_deref_instr_ptr;
  }

  DECL_VALUE_ENCODER_C0_FN
  DECL_CONSTANT_ENCODER_C0_FN
};

/// Event that reads `sizeof(T)` bytes from memory
template<typename T>
class ReadEvent : public Event {
private:
  template<typename U>
  friend std::unique_ptr<ReadEvent<U>> internal_make_read_event(
    const Zone& zone, EventId event_id);

  ReadEvent(EventId event_id, ThreadId thread_id, const Zone& zone,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(event_id, thread_id, zone, true, &TypeInfo<T>::s_type, condition_ptr) {}

public:
  ReadEvent(ThreadId thread_id, const Zone& zone,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(thread_id, zone, true, &TypeInfo<T>::s_type, condition_ptr) {}

  ~ReadEvent() {}

  DECL_VALUE_ENCODER_C0_FN
  DECL_CONSTANT_ENCODER_C0_FN
};

/// \internal Event for thread synchronization
class SyncEvent : public Event {
private:
  typedef bool Sync;

  DECL_VALUE_ENCODER_C0_FN
  DECL_CONSTANT_ENCODER_C0_FN

protected:
  SyncEvent(ThreadId thread_id, const Zone& zone, bool receive,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(thread_id, zone, receive, &TypeInfo<Sync>::s_type, condition_ptr) {}
};

/// \internal Write event for thread synchronization

/// Synchronization occurs through a unique \ref Event::zone() "zone atom".
class SendEvent : public SyncEvent {
public:
  SendEvent(ThreadId thread_id,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    SyncEvent(thread_id, Zone::unique_atom(), false, condition_ptr) {}
};

/// \internal Read event for thread synchronization
class ReceiveEvent : public SyncEvent {
public:
  /// Event that reads from the given zone, preferably SendEvent::zone()
  ReceiveEvent(ThreadId thread_id, const Zone& zone,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    SyncEvent(thread_id, zone, true, condition_ptr) {}
};

}

#endif
