// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_VAR_H_
#define LIBSE_CONCURRENT_VAR_H_

#include <utility>
#include <memory>

#include "concurrent/event.h"
#include "concurrent/instr.h"
#include "concurrent/recorder.h"

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
std::unique_ptr<ReadEvent<T>> make_read_event(const MemoryAddr& addr) {
  const unsigned thread_id = recorder_ptr()->thread_id();
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(recorder_ptr()->
    path_condition().top());

  return std::unique_ptr<ReadEvent<T>>(new ReadEvent<T>(thread_id, addr,
    condition_ptr));
}

/// Common data about a program variable of type `T`

/// Every object has a \ref BaseDeclVar::addr() "memory address" that identifies
/// the memory regions that are postulated to be affected through assignments to
/// the variable. The memory address is guaranteed to be unique for thread-local
/// and shared variables. Even temporary variables (allocated on the call stack)
/// are enforced to have their own unique memory address. Note, for pointers,
/// BaseDeclVar::addr() only defines the address at which the pointer resides,
/// not the memory to which it points.
///
/// If a BaseDeclVar is \ref MemoryAddr::is_shared() "shared", its lifetime must
/// span all threads that can access access it. Otherwise, the variable is said
/// to be thread-local. Typically, sharing is achieved by allocating an object
/// statically. Such variable declarations are always initialized to zero by the
/// "main thread" unless another initial value is explicitly given. Noteworthy,
/// the value of any variable, thread-local or shared, may also be symbolic.
///
/// \remark Variables shared between threads should be statically allocated
template<typename T>
class BaseDeclVar {
private:
  const MemoryAddr m_addr;

  // Never null
  std::shared_ptr<DirectWriteEvent<T>> m_direct_write_event_ptr;

  static constexpr unsigned main_thread_id = 0;

  template<typename U>
  static std::unique_ptr<DirectWriteEvent<U>> make_direct_write_event(
    const MemoryAddr& addr) {

    std::unique_ptr<ReadInstr<U>> zero_instr_ptr(new LiteralReadInstr<U>());
    return std::unique_ptr<DirectWriteEvent<U>>(new DirectWriteEvent<U>(main_thread_id,
      addr, std::move(zero_instr_ptr)));
  }

protected:
  /// Declare a new variable of type `T`

  /// \param is_shared - can other threads modify the variable?
  ///
  /// Once declared, the memory address of the variable never changes.
  /// Each variable is initialized to zero. This is accomplished through
  /// a direct write event from within the main thread.
  BaseDeclVar(bool is_shared) : m_addr(MemoryAddr::alloc<T>(is_shared)),
    m_direct_write_event_ptr(make_direct_write_event<T>(m_addr)) {}

  ~BaseDeclVar() {}

public:
  const MemoryAddr& addr() const { return m_addr; }

  const DirectWriteEvent<T>& direct_write_event_ref() const {
    return *m_direct_write_event_ptr;
  }

  void set_direct_write_event_ptr(const std::shared_ptr<DirectWriteEvent<T>>& event_ptr) {
    m_direct_write_event_ptr = event_ptr;
  }
};

/// Variable declaration allowing only direct memory writes
template<typename T>
class DeclVar : public BaseDeclVar<T> {
public:
  typedef T DirectType;
  typedef T IndirectType;

  /// Declare a variable that only allows direct memory writes

  /// \param is_shared - can other threads modify the variable?
  DeclVar(bool is_shared) : BaseDeclVar<T>(is_shared) {}

  ~DeclVar() {}
};

/// Fixed-sized array declaration
template<typename T, size_t N>
class DeclVar<T[N]> : public BaseDeclVar<T*> {
private:
  const MemoryAddr m_array_addr;
  std::shared_ptr<IndirectWriteEvent<T, size_t, N>> m_indirect_write_event_ptr;

public:
  typedef T* DirectType;
  typedef T IndirectType;

  /// Declare a fixed-sized array

  /// \param is_array_shared - can other threads modify any array elements?
  DeclVar(bool is_array_shared) :
    BaseDeclVar<T*>(/* base pointer cannot be modified */ false),
    m_array_addr(MemoryAddr::alloc<T>(is_array_shared, N)),
    m_indirect_write_event_ptr() {}

  ~DeclVar() {}

  const MemoryAddr& array_addr() { return m_array_addr; }

  const IndirectWriteEvent<T, size_t, N>& indirect_write_event_ref() const {
    return *m_indirect_write_event_ptr;
  }

  void set_indirect_write_event_ptr(
    const std::shared_ptr<IndirectWriteEvent<T, size_t, N>>& event_ptr) {

    m_indirect_write_event_ptr = event_ptr;
  }
};

/// \internal
template<typename Range, typename Domain, size_t N>
class Memory {
private:
  template<typename _Range, typename _Domain, size_t _N>
  friend class LocalMemory;

  template<typename _Range, typename _Domain, size_t _N>
  friend class SharedMemory;

  const MemoryAddr m_element_addr;

  // Both pointers are never null
  DeclVar<Range[N]>* const m_var_ptr;
  std::unique_ptr<DerefReadInstr<Range[N], Domain>> m_deref_instr_ptr;

  Memory(DeclVar<Range[N]>* const var_ptr, const MemoryAddr& element_addr,
    std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr) :
    m_var_ptr(var_ptr), m_element_addr(element_addr),
    m_deref_instr_ptr(std::move(deref_instr_ptr)) {

    assert(nullptr != m_var_ptr);
    assert(nullptr != m_deref_instr_ptr);
  }

  Memory(Memory&& other) : m_var_ptr(other.m_var_ptr),
    m_element_addr(other.m_element_addr),
    m_deref_instr_ptr(std::move(other.m_deref_instr_ptr)) {}

  const MemoryAddr& addr() const { return m_var_ptr->addr(); }

  void operator=(std::unique_ptr<ReadInstr<Range>> instr_ptr) {
    assert(nullptr != instr_ptr);

    const std::shared_ptr<IndirectWriteEvent<Range, Domain, N>> write_event_ptr(
      recorder_ptr()->instr(m_element_addr, std::move(m_deref_instr_ptr),
        std::move(instr_ptr)));
    m_var_ptr->set_indirect_write_event_ptr(write_event_ptr);
  }
};

/// \internal
template<typename T>
class LocalRead {
private:
  template<typename Range, typename Domain, size_t N>
  friend class LocalMemory;

  template<typename U> friend class LocalVar;

  // Must be never null
  std::shared_ptr<ReadEvent<T>> m_read_event_ptr;

  LocalRead(std::shared_ptr<ReadEvent<T>> read_event_ptr) :
    m_read_event_ptr(read_event_ptr) {

    assert(nullptr != read_event_ptr);
  }
};

/// \internal
template<typename Range, typename Domain, size_t N>
class LocalMemory {
private:
  template<typename U> friend class LocalVar;

  Memory<Range, Domain, N> m_memory;
  LocalRead<Range[N]>* const m_local_read_ptr;
 
  LocalMemory(DeclVar<Range[N]>* const var_ptr, const MemoryAddr& element_addr,
    std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr,
    LocalRead<Range[N]>* const local_read_ptr) :
    m_memory(var_ptr, element_addr, std::move(deref_instr_ptr)),
    m_local_read_ptr(local_read_ptr) {

    assert(nullptr != m_local_read_ptr);
  }

  LocalMemory(LocalMemory&& other) : m_memory(std::move(other.m_memory)),
    m_local_read_ptr(other.m_local_read_ptr) {}

public:
  std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr() {
    return std::move(m_memory.m_deref_instr_ptr);
  }

  void operator=(std::unique_ptr<ReadInstr<Range>> instr_ptr) {
    m_memory = std::move(instr_ptr);

    m_local_read_ptr->m_read_event_ptr = make_read_event<Range[N]>(m_memory.addr());
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
  LocalVar() : m_var(false), m_local_read(make_read_event<T>(m_var.addr())) {}

  const MemoryAddr& addr() const { return m_var.addr(); }

  std::shared_ptr<ReadEvent<T>> read_event_ptr() const {
    return m_local_read.m_read_event_ptr;
  }

  const DirectWriteEvent<typename DeclVar<T>::DirectType>&
  direct_write_event_ref() const {
    return m_var.direct_write_event_ref();
  }

  LocalVar<T>& operator=(const LocalVar<T>& local_var) {
    return operator=(alloc_read_instr(local_var));
  }

  LocalVar<T>& operator=(const SharedVar<T>& shared_var) {
    return operator=(alloc_read_instr(shared_var));
  }

  LocalVar<T>& operator=(const T literal) {
    return operator=(alloc_read_instr(literal));
  }

  LocalVar<T>& operator=(std::unique_ptr<ReadInstr<T>> instr_ptr) {
    const std::shared_ptr<DirectWriteEvent<T>> write_event_ptr(
      recorder_ptr()->instr(addr(), std::move(instr_ptr)));

    m_var.set_direct_write_event_ptr(write_event_ptr);
    m_local_read.m_read_event_ptr = make_read_event<T>(addr());

    return *this;
  }

  template<size_t N = std::extent<T>::value,
    class = typename std::enable_if<std::is_array<T>::value and 0 < N>::type>
  LocalMemory<typename std::remove_extent<T>::type, size_t, N>
  operator[](size_t offset) {

    typedef typename std::remove_extent<T>::type Range;
    typedef size_t Domain;

    std::unique_ptr<ReadInstr<Domain>> offset_ptr(
      new LiteralReadInstr<Domain>(offset));

    std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr(
      new DerefReadInstr<Range[N], Domain>(alloc_read_instr(*this),
        std::move(offset_ptr)));

    const MemoryAddr offset_addr = m_var.array_addr() + offset;
    return LocalMemory<Range, Domain, N>(&m_var, offset_addr,
      std::move(deref_instr_ptr), &m_local_read);
  }

  template<typename U = T, size_t N = std::extent<U>::value,
    class = typename std::enable_if<std::is_array<U>::value and 0 < N>::type>
  const IndirectWriteEvent</* range */ typename DeclVar<U>::IndirectType,
    /* domain */ size_t, N>& indirect_write_event_ref() const {

    return m_var.indirect_write_event_ref();
  }
};

/// \internal
template<typename Range, typename Domain, size_t N>
class SharedMemory {
private:
  template<typename U> friend class SharedVar;

  Memory<Range, Domain, N> m_memory;
 
  SharedMemory(DeclVar<Range[N]>* const var_ptr, const MemoryAddr& element_addr,
    std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr) :
    m_memory(var_ptr, element_addr, std::move(deref_instr_ptr)) {}

  SharedMemory(SharedMemory&& other) : m_memory(std::move(other.m_memory)) {}

public:
  void operator=(std::unique_ptr<ReadInstr<Range>> instr_ptr) {
    m_memory = std::move(instr_ptr);
  }

  template<typename U>
  void operator=(const U& v) { operator=(alloc_read_instr(v)); }
};

/// Shared variable accessible to multiple threads
template<typename T>
class SharedVar {
private:
  DeclVar<T> m_var;

public:
  SharedVar() : m_var(true) {}

  const MemoryAddr& addr() const { return m_var.addr(); }

  const DirectWriteEvent<typename DeclVar<T>::DirectType>&
  direct_write_event_ref() const {
    return m_var.direct_write_event_ref();
  }

  SharedVar<T>& operator=(const LocalVar<T>& local_var) {
    return operator=(alloc_read_instr(local_var));
  }

  SharedVar<T>& operator=(const SharedVar<T>& shared_var) {
    return operator=(alloc_read_instr(shared_var));
  }

  SharedVar<T>& operator=(const T literal) {
    return operator=(alloc_read_instr(literal));
  }

  SharedVar<T>& operator=(std::unique_ptr<ReadInstr<T>> instr_ptr) {
    const std::shared_ptr<DirectWriteEvent<T>> write_event_ptr(
      recorder_ptr()->instr(addr(), std::move(instr_ptr)));

    m_var.set_direct_write_event_ptr(write_event_ptr);

    return *this;
  }

  template<size_t N = std::extent<T>::value,
    class = typename std::enable_if<std::is_array<T>::value and 0 < N>::type>
  SharedMemory<typename std::remove_extent<T>::type, size_t, N>
  operator[](size_t offset) {

    typedef typename std::remove_extent<T>::type Range;
    typedef size_t Domain;

    std::unique_ptr<ReadInstr<Domain>> offset_ptr(
      new LiteralReadInstr<Domain>(offset));

    std::unique_ptr<DerefReadInstr<Range[N], Domain>> deref_instr_ptr(
      new DerefReadInstr<Range[N], Domain>(alloc_read_instr(*this),
        std::move(offset_ptr)));

    const MemoryAddr offset_addr = m_var.array_addr() + offset;
    return SharedMemory<Range, Domain, N>(&m_var, offset_addr,
      std::move(deref_instr_ptr));
  }

  template<typename U = T, size_t N = std::extent<U>::value,
    class = typename std::enable_if<std::is_array<U>::value and 0 < N>::type>
  const IndirectWriteEvent</* range */ typename DeclVar<U>::IndirectType,
    /* domain */ size_t, N>& indirect_write_event_ref() const {

    return m_var.indirect_write_event_ref();
  }
};

}

#endif
