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

template<typename T> class DeclVar;

template<typename T>
std::unique_ptr<ReadInstr<T>> alloc_read_instr(const DeclVar<T>& var);

template<typename T>
std::unique_ptr<ReadInstr<typename std::enable_if<
  std::is_arithmetic<T>::value, T>::type>> alloc_read_instr(const T& literal);

/// Program variable declaration of type `T`

/// Any declared variable that should be part of the analysis must be of type
/// BaseDeclVar. Every object has a \ref BaseDeclVar::addr() "memory address"
/// that identifies the memory regions that are postulated to be affected by
/// the assignment operator of the subclass. The memory address is guaranteed
/// to be unique across all threads even for temporary variables allocated on
/// the call stack. Note that for pointers, BaseDeclVar::addr() only refers to
/// the address at which the pointer resides, not the memory to which it points.
///
/// If a BaseDeclVar is \ref MemoryAddr::is_shared() "shared", its lifetime must
/// span all threads that can access access it. Typically, sharing is achieved
/// by allocating a BaseDeclVar object statically. This common use case explains
/// why BaseDeclVar objects are always initialized to zero by the "main thread"
/// unless another initial value is explicitly given to the constructor of the
/// subclass. Note that the value of any declared variable may also be symbolic.
///
/// \remark Variables shared between threads should be statically allocated
template<typename T>
class BaseDeclVar {
protected:
  const MemoryAddr m_addr;

  /// Declare a new variable of type `T`

  /// \param addr - memory address affected by writing the variable
  /// \param is_shared - can other threads modify the variable?
  ///
  /// Note that the memory address of the variable never changes.
  BaseDeclVar(const MemoryAddr& addr) : m_addr(addr) {}

public:
  const MemoryAddr& addr() const { return m_addr; }
};

template<typename T>
static std::unique_ptr<WriteEvent<T>> init_write_event(const MemoryAddr& addr) {
  static constexpr unsigned main_thread_id = 0;
  std::unique_ptr<ReadInstr<T>> zero_instr_ptr(new LiteralReadInstr<T>());
  return std::unique_ptr<WriteEvent<T>>(new WriteEvent<T>(main_thread_id,
    addr, std::move(zero_instr_ptr)));
}

/// Scalar variable declaration
template<typename T>
class DeclVar : public BaseDeclVar<T> {
private:
  std::shared_ptr<WriteEvent<T>> m_event_ptr;

public:
  using BaseDeclVar<T>::m_addr;

  DeclVar(bool is_shared = true) :
    BaseDeclVar<T>(MemoryAddr::alloc<T>(is_shared)),
    m_event_ptr(init_write_event<T>(m_addr)) {}

  const WriteEvent<T>& write_event_ref() const { return *m_event_ptr; }

  DeclVar<T>& operator=(const DeclVar<T>& other) {
    return operator=(alloc_read_instr(other));
  }

  DeclVar<T>& operator=(std::unique_ptr<ReadInstr<T>> instr_ptr) {
    m_event_ptr = recorder_ptr()->instr(m_addr, std::move(instr_ptr));
    return *this;
  }
};

template<typename T, typename U, size_t N>
class Memory {
private:
  const MemoryAddr m_addr;
  std::shared_ptr<WriteEvent<T>>* m_event_ptr;

  // Never null
  std::unique_ptr<ReadInstr<T>> m_deref_instr_ptr;

  friend class DeclVar<T[N]>;

  Memory(std::shared_ptr<WriteEvent<T>>* event_ptr,
    const MemoryAddr& addr, std::unique_ptr<ReadInstr<T>> deref_instr_ptr) :
    m_event_ptr(event_ptr), m_addr(addr),
    m_deref_instr_ptr(std::move(deref_instr_ptr)) {

    assert(nullptr != m_deref_instr_ptr);
  }

  Memory(Memory&& other) : m_addr(other.m_addr), m_event_ptr(other.m_event_ptr),
    m_deref_instr_ptr(std::move(other.m_deref_instr_ptr)) {}

public:
  // TODO: Fix return type in presence of move semantics
  void operator=(std::unique_ptr<ReadInstr<T>> instr_ptr) {
    assert(nullptr != instr_ptr);

    *m_event_ptr = recorder_ptr()->instr(m_addr,
      std::move(m_deref_instr_ptr), std::move(instr_ptr));
  }

  // TODO: Fix return type in presence of move semantics
  template<typename V>
  void operator=(const V& literal) {
    operator=(alloc_read_instr(literal));
  }
};

/// Fixed-sized array declaration
template<typename T, size_t N>
class DeclVar<T[N]> : public BaseDeclVar<T[N]> {
private:
  std::shared_ptr<WriteEvent<T>> m_event_ptr;
  const MemoryAddr m_array_addr;

public:
  using BaseDeclVar<T[N]>::m_addr;

  DeclVar(bool is_shared = true) :
    BaseDeclVar<T[N]>(MemoryAddr::alloc<T*>(false)),
    m_event_ptr(init_write_event<T>(m_addr)),
    m_array_addr(MemoryAddr::alloc<T>(is_shared, N)) {}

  const WriteEvent<T>& write_event_ref() const { return *m_event_ptr; }

  Memory<T, size_t, N> operator[](size_t offset) {
    std::unique_ptr<ReadInstr<size_t>> offset_ptr(
      new LiteralReadInstr<size_t>(offset));

    std::unique_ptr<ReadInstr<T>> deref_instr_ptr(
      new DereferenceReadInstr<T[N], size_t>(
        alloc_read_instr(*this), std::move(offset_ptr)));

    const MemoryAddr offset_addr = m_array_addr + offset;
    return Memory<T, size_t, N>(&m_event_ptr, offset_addr,
      std::move(deref_instr_ptr));
  }
};

}

#endif
