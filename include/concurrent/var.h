// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_VAR_H_
#define LIBSE_CONCURRENT_VAR_H_

#include <utility>
#include <memory>

#include "concurrent/event.h"
#include "concurrent/instr.h"
#include "concurrent/thread.h"

namespace se {

template<typename T> class LocalVar;
template<typename T> class SharedVar;

template<typename T>
std::unique_ptr<ReadInstr<T>> alloc_read_instr(const LocalVar<T>& var);

template<typename T>
std::unique_ptr<ReadInstr<T>> alloc_read_instr(const SharedVar<T>& var);

template<typename T>
std::unique_ptr<ReadInstr<typename std::enable_if<
  std::is_arithmetic<T>::value, T>::type>> alloc_read_instr(const T& literal);

template<typename T>
std::unique_ptr<ReadEvent<T>> make_read_event(const Zone& zone) {
  return std::unique_ptr<ReadEvent<T>>(new ReadEvent<T>(ThisThread::thread_id(),
    zone, ThisThread::path_condition_ptr()));
}

template<typename T>
std::unique_ptr<ReadEvent<T>> internal_make_read_event(const Zone& zone,
  unsigned event_id) {

  const unsigned thread_id = ThisThread::thread_id();
  return std::unique_ptr<ReadEvent<T>>(new ReadEvent<T>(event_id, thread_id,
    zone, ThisThread::path_condition_ptr()));
}

/// Variable declaration allowing only direct memory writes

/// Every object has a \ref DeclVar<T>::zone() "zone" that allows events to be
/// linked up according to the partial order encoding axioms. In the case of
/// pointers and arrays, the variable's zone is the same as any dereferenced
/// memory reachable through it, e.g. `p[3].addr() == p.addr()` for a pointer.
///
/// If a DeclVar<T>'s zone is not \ref Zone::is_bottom() "bottom", the variable
/// is said to be "shared". Otherwise, the variable is called "thread-local".
/// The lifetime of a shared variable must span across all threads that can
/// access it. Also the zone of each shared variable is guaranteed to be unique.
///
/// Typically, sharing is achieved by allocating an object statically. Therefore,
/// shared variables are always initialized to zero by the thread in which they
/// are defined --- unless, of course, another initial value is explicitly given.
/// Noteworthy, the value of any variable, thread-local or shared, may also be
/// symbolic.
///
/// \remark Variables shared between threads should be statically allocated
template<typename T>
class DeclVar {
private:
  const Zone m_zone;

  // Most recent direct write event, never null
  std::shared_ptr<DirectWriteEvent<T>> m_direct_write_event_ptr;

  template<typename U>
  static std::shared_ptr<DirectWriteEvent<U>> make_direct_write_event(
    const Zone& zone, std::unique_ptr<ReadInstr<U>> instr_ptr) {

    const std::shared_ptr<DirectWriteEvent<U>> direct_write_event_ptr(
      new DirectWriteEvent<U>(ThisThread::thread_id(), zone,
        std::move(instr_ptr)));

    return direct_write_event_ptr;
  }

public:
  /// Label to link up events
  const Zone& zone() const { return m_zone; }

  /// Declare a variable of type `T` that only allows direct memory writes

  /// \param is_shared - can other threads modify the variable?
  /// \param instr_ptr - initialization instruction
  ///
  /// The newly declared variable is initialized according to the given
  /// instruction that is defined to execute within the current thread.
  DeclVar(bool is_shared, std::unique_ptr<ReadInstr<T>> instr_ptr) :
    m_zone(is_shared ? Zone::unique_atom() : Zone::bottom()),
    m_direct_write_event_ptr(make_direct_write_event<T>(m_zone,
      std::move(instr_ptr))) {

    Threads::slice().append(ThisThread::thread_id(), m_direct_write_event_ptr);
  }

  /// Declare a variable of type `T` that only allows direct memory writes

  /// \param is_shared - can other threads modify the variable?
  /// \param v - initialization value
  ///
  /// The newly declared variable is initialized to `v`. This is accomplished
  /// through a direct write event from within the current thread.
  DeclVar(bool is_shared, const T v = 0) :
    m_zone(is_shared ? Zone::unique_atom() : Zone::bottom()),
    m_direct_write_event_ptr(make_direct_write_event<T>(m_zone,
      std::unique_ptr<ReadInstr<T>>(new LiteralReadInstr<T>(v)))) {

    Threads::slice().append(ThisThread::thread_id(), m_direct_write_event_ptr);
  }

  ~DeclVar() {}

  const DirectWriteEvent<T>& direct_write_event_ref() const {
    return *m_direct_write_event_ptr;
  }

  std::shared_ptr<DirectWriteEvent<T>> direct_write_event_ptr() const {
    return m_direct_write_event_ptr;
  }

  /// \pre argument must not be null
  void set_direct_write_event_ptr(const std::shared_ptr<DirectWriteEvent<T>>& event_ptr) {
    assert(nullptr != event_ptr);
    m_direct_write_event_ptr = event_ptr;
  }
};

/// Fixed-sized array declaration
template<typename T, size_t N>
class DeclVar<T[N]> {
static_assert(0 < N, "N must be greater than zero");

private:
  const Zone m_zone;
  const std::shared_ptr<DirectWriteEvent<T[N]>> m_direct_write_event_ptr;
  std::shared_ptr<IndirectWriteEvent<T, size_t, N>> m_indirect_write_event_ptr;

  template<typename U>
  static std::shared_ptr<DirectWriteEvent<U>> make_direct_write_event(
    const Zone& zone) {

    std::unique_ptr<ReadInstr<U>> instr_ptr(new LiteralReadInstr<U>());
    const std::shared_ptr<DirectWriteEvent<U>> direct_write_event_ptr(
      new DirectWriteEvent<U>(ThisThread::thread_id(), zone,
        std::move(instr_ptr)));

    return direct_write_event_ptr;
  }

public:
  const Zone& zone() const { return m_zone; }

  /// Declare a fixed-sized array

  /// \param is_shared - can other threads modify any array elements?
  DeclVar(bool is_shared) :
    m_zone(is_shared ? Zone::unique_atom() : Zone::bottom()),
    m_direct_write_event_ptr(make_direct_write_event<T[N]>(m_zone)),
    m_indirect_write_event_ptr() {

    Threads::slice().append(ThisThread::thread_id(), m_direct_write_event_ptr);
  }

  ~DeclVar() {}

  const DirectWriteEvent<T[N]>& direct_write_event_ref() const {
    return *m_direct_write_event_ptr;
  }

  const IndirectWriteEvent<T, size_t, N>& indirect_write_event_ref() const {
    return *m_indirect_write_event_ptr;
  }

  void set_indirect_write_event_ptr(
    const std::shared_ptr<IndirectWriteEvent<T, size_t, N>>& event_ptr) {

    m_indirect_write_event_ptr = event_ptr;
  }
};

/// \internal Load and store memory at an address
template<typename Range, typename Domain, size_t N>
class Memory {
private:
  template<typename _Range, typename _Domain, size_t _N>
  friend class LocalMemory;

  template<typename _Range, typename _Domain, size_t _N>
  friend class SharedMemory;

  // Both pointers are never null
  DeclVar<Range[N]>* const m_var_ptr;
  std::unique_ptr<DerefReadInstr<Range[N], Domain>> m_deref_instr_ptr;

  /// Memory accessible through a deference instruction

  /// \param var_ptr - variable that is affected by an indirect write
  /// \param deref_instr_ptr - instruction to load an array element
  Memory(DeclVar<Range[N]>* const var_ptr,
    std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr) :
    m_var_ptr(var_ptr), m_deref_instr_ptr(std::move(deref_instr_ptr)) {

    assert(nullptr != m_var_ptr);
    assert(nullptr != m_deref_instr_ptr);
  }

  Memory(Memory&& other) : m_var_ptr(other.m_var_ptr),
    m_deref_instr_ptr(std::move(other.m_deref_instr_ptr)) {}

  const Zone& zone() const { return m_var_ptr->zone(); }

  void store(std::unique_ptr<ReadInstr<Range>> instr_ptr) {
    assert(nullptr != instr_ptr);

    const std::shared_ptr<IndirectWriteEvent<Range, Domain, N>> write_event_ptr(
      ThisThread::instr(zone(), std::move(m_deref_instr_ptr),
        std::move(instr_ptr)));

    m_var_ptr->set_indirect_write_event_ptr(write_event_ptr);
  }
};

/// \internal
template<typename Range, typename Domain, size_t N>
class SharedMemory {
private:
  template<typename U> friend class SharedVar;

  Memory<Range, Domain, N> m_memory;
 
  SharedMemory(DeclVar<Range[N]>* const var_ptr,
    std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr) :
    m_memory(var_ptr, std::move(deref_instr_ptr)) {}

  SharedMemory(SharedMemory&& other) : m_memory(std::move(other.m_memory)) {}

public:
  const Zone& zone() const { return m_memory.zone(); }

  std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr() {
    return std::move(m_memory.m_deref_instr_ptr);
  }

  void operator=(std::unique_ptr<ReadInstr<Range>> instr_ptr) {
    m_memory.store(std::move(instr_ptr));
  }

  template<typename U>
  void operator=(const U& v) { operator=(alloc_read_instr(v)); }
};

/// \internal Has a ReadEvent<T> pointer if and only if T is directly writable
template<typename T>
class LocalRead {
private:
  std::shared_ptr<ReadEvent<T>> m_read_event_ptr;

public:
  LocalRead(std::shared_ptr<ReadEvent<T>> read_event_ptr) :
    m_read_event_ptr(read_event_ptr) { assert(nullptr != read_event_ptr); }

  std::shared_ptr<ReadEvent<T>> read_event_ptr() const {
    return m_read_event_ptr;
  }

  /// \pre: argument is never null
  void set_read_event_ptr(const std::shared_ptr<ReadEvent<T>>& read_event_ptr) {
    assert(nullptr != read_event_ptr);
    m_read_event_ptr = read_event_ptr;
  }
};

/// \internal
template<typename Range, typename Domain, size_t N>
class LocalMemory {
private:
  template<typename U> friend class LocalVar;

  Memory<Range, Domain, N> m_memory;
  LocalRead<Range[N]>* const m_local_read_ptr;
 
  LocalMemory(DeclVar<Range[N]>* const var_ptr,
    std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr,
    LocalRead<Range[N]>* const local_read_ptr) :
    m_memory(var_ptr, std::move(deref_instr_ptr)),
    m_local_read_ptr(local_read_ptr) {

    assert(nullptr != m_local_read_ptr);
  }

  LocalMemory(LocalMemory&& other) : m_memory(std::move(other.m_memory)),
    m_local_read_ptr(other.m_local_read_ptr) {}

public:
  const Zone& zone() const { return m_memory.zone(); }

  std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr() {
    return std::move(m_memory.m_deref_instr_ptr);
  }

  void operator=(std::unique_ptr<ReadInstr<Range>> instr_ptr) {
    m_memory.store(std::move(instr_ptr));

    m_local_read_ptr->set_read_event_ptr(internal_make_read_event<Range[N]>(
      zone(), m_memory.m_var_ptr->indirect_write_event_ref().event_id()));
  }

  template<typename U>
  void operator=(const U& v) { operator=(alloc_read_instr(v)); }
};

/// Thread-local variable
template<typename T>
class LocalVar {
private:
  DeclVar<T> m_var;
  LocalRead<T> m_local_read;

public:
  LocalVar() : m_var(false), m_local_read(internal_make_read_event<T>(
    m_var.zone(), m_var.direct_write_event_ref().event_id())) {}

  LocalVar(const T v) : m_var(false, v),
    m_local_read(internal_make_read_event<T>(m_var.zone(),
      m_var.direct_write_event_ref().event_id())) {}

  LocalVar(const LocalVar& other) : m_var(false, alloc_read_instr(other)),
    m_local_read(internal_make_read_event<T>(m_var.zone(),
      m_var.direct_write_event_ref().event_id())) {}

  LocalVar(const SharedVar<T>& other) : m_var(false, alloc_read_instr(other)),
    m_local_read(internal_make_read_event<T>(m_var.zone(),
      m_var.direct_write_event_ref().event_id())) {

    Threads::slice().append_all(ThisThread::thread_id(),
      m_var.direct_write_event_ref().instr_ref());
    Threads::slice().append(ThisThread::thread_id(),
      m_var.direct_write_event_ptr());
  }

  const Zone& zone() const { return m_var.zone(); }

  std::shared_ptr<ReadEvent<T>> read_event_ptr() const {
    return m_local_read.read_event_ptr();
  }

  const DirectWriteEvent<T>& direct_write_event_ref() const {
    return m_var.direct_write_event_ref();
  }

  LocalVar<T>& operator=(const LocalVar<T>& local_var) {
    return operator=(alloc_read_instr(local_var));
  }

  template<typename Domain, size_t N>
  LocalVar<T>& operator=(LocalMemory<T, Domain, N>&& local_memory) {
    return operator=(alloc_read_instr(std::move(local_memory)));
  }

  template<typename Domain, size_t N>
  LocalVar<T>& operator=(SharedMemory<T, Domain, N>&& shared_memory) {
    return operator=(alloc_read_instr(std::move(shared_memory)));
  }

  LocalVar<T>& operator=(const SharedVar<T>& shared_var) {
    return operator=(alloc_read_instr(shared_var));
  }

  LocalVar<T>& operator=(const T v) { return operator=(alloc_read_instr(v)); }

  LocalVar<T>& operator=(std::unique_ptr<ReadInstr<T>> instr_ptr) {
    const std::shared_ptr<DirectWriteEvent<T>> write_event_ptr(
      ThisThread::instr(zone(), std::move(instr_ptr)));

    m_var.set_direct_write_event_ptr(write_event_ptr);
    m_local_read.set_read_event_ptr(internal_make_read_event<T>(zone(),
      write_event_ptr->event_id()));

    return *this;
  }

  template<size_t N = std::extent<T>::value,
    class = typename std::enable_if<std::is_array<T>::value and 0 < N>::type>
  LocalMemory<typename std::remove_extent<T>::type, size_t, N>
  operator[](size_t index) {

    typedef typename std::remove_extent<T>::type Range;
    typedef size_t Domain;

    std::unique_ptr<ReadInstr<Domain>> index_read_instr_ptr(
      new LiteralReadInstr<Domain>(index));

    std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr(
      new DerefReadInstr<Range[N], Domain>(alloc_read_instr(*this),
        std::move(index_read_instr_ptr)));

    return LocalMemory<Range, Domain, N>(&m_var, std::move(deref_instr_ptr),
      &m_local_read);
  }

  template<typename U = T, size_t N = std::extent<U>::value,
    class = typename std::enable_if<std::is_array<U>::value and 0 < N>::type>
  const IndirectWriteEvent</* range */ typename std::remove_extent<T>::type,
    /* domain */ size_t, N>& indirect_write_event_ref() const {

    return m_var.indirect_write_event_ref();
  }
};

/// Shared variable accessible to multiple threads
template<typename T>
class SharedVar {
private:
  DeclVar<T> m_var;

public:
  SharedVar() : m_var(true) {}
  SharedVar(const T v) : m_var(true, v) {}

  const Zone& zone() const { return m_var.zone(); }

  const DirectWriteEvent<T>& direct_write_event_ref() const {
    return m_var.direct_write_event_ref();
  }

  SharedVar<T>& operator=(const LocalVar<T>& local_var) {
    return operator=(alloc_read_instr(local_var));
  }

  SharedVar<T>& operator=(const SharedVar<T>& shared_var) {
    return operator=(alloc_read_instr(shared_var));
  }

  template<typename Domain, size_t N>
  SharedVar<T>& operator=(SharedMemory<T, Domain, N>&& shared_memory) {
    return operator=(alloc_read_instr(std::move(shared_memory)));
  }

  SharedVar<T>& operator=(const T v) { return operator=(alloc_read_instr(v)); }

  SharedVar<T>& operator=(std::unique_ptr<ReadInstr<T>> instr_ptr) {
    const std::shared_ptr<DirectWriteEvent<T>> write_event_ptr(
      ThisThread::instr(zone(), std::move(instr_ptr)));

    m_var.set_direct_write_event_ptr(write_event_ptr);

    return *this;
  }

  /// Literal index
  template<size_t N = std::extent<T>::value,
    class = typename std::enable_if<std::is_array<T>::value and 0 < N>::type>
  SharedMemory<typename std::remove_extent<T>::type, size_t, N>
  operator[](size_t index) {

    typedef typename std::remove_extent<T>::type Range;
    typedef size_t Domain;

    std::unique_ptr<ReadInstr<Domain>> index_read_instr_ptr(
      new LiteralReadInstr<Domain>(index));

    std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr(
      new DerefReadInstr<Range[N], Domain>(alloc_read_instr(*this),
        std::move(index_read_instr_ptr)));

    return SharedMemory<Range, Domain, N>(&m_var, std::move(deref_instr_ptr));
  }

  /// Potentially symbolic index
  template<size_t N = std::extent<T>::value,
    class = typename std::enable_if<std::is_array<T>::value and 0 < N>::type>
  SharedMemory<typename std::remove_extent<T>::type, size_t, N>
  operator[](SharedVar<size_t> index) {

    typedef typename std::remove_extent<T>::type Range;
    typedef size_t Domain;

    std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr(
      new DerefReadInstr<Range[N], Domain>(alloc_read_instr(*this),
        alloc_read_instr(index)));

    return SharedMemory<Range, Domain, N>(&m_var, std::move(deref_instr_ptr));
  }

  template<typename U = T, size_t N = std::extent<U>::value,
    class = typename std::enable_if<std::is_array<U>::value and 0 < N>::type>
  const IndirectWriteEvent</* range */ typename std::remove_extent<T>::type,
    /* domain */ size_t, N>& indirect_write_event_ref() const {

    return m_var.indirect_write_event_ref();
  }
};

template<typename T>
std::unique_ptr<ReadInstr<T>> any() {
  return std::unique_ptr<ReadInstr<T>>(new BasicReadInstr<T>(
    make_read_event<T>(/* thread-local */ Zone::bottom())));
}

}

#endif
