// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef MEMORY_H_
#define MEMORY_H_

#include "var.h"
#include "instr.h"

namespace se {

/// A single or multiple contiguous scalar symbolic memory blocks
template<typename T>
class Memory {
private:
  const size_t m_size;

  SharedExpr m_array_expr;
  Version m_version;

public:
  /// Create a memory region that may hold up to @size scalar symbolic values.
  Memory(size_t size, const std::string& identifier) : m_size(size),
    m_array_expr(new ArrayExpr(TypeConstructor<T>::type, size, identifier)),
    m_version(VZERO) {}

  Version version() const { return m_version; }

  Value<T> load_value(const Value<size_t>& index) const {
    const SharedExpr select_expr(new SelectExpr(m_array_expr, index.expr()));
    return Value<T>(0, select_expr);
  }

  void store_value(const Value<size_t>& index, const Value<T>& value) {
    m_version++;
    m_array_expr = SharedExpr(new StoreExpr(m_array_expr,
                     index.expr(), value.expr()));
  }
};

/// A single scalar symbolic memory block
template<typename T>
class MemoryBlock : public ScalarVar<T> {
private:
  const std::shared_ptr<Memory<T>> m_memory;
  const Value<size_t> m_index;

public:
  MemoryBlock(const std::shared_ptr<Memory<T>>& memory,
    const Value<size_t>& index) : ScalarVar<T>(memory->load_value(index),
    memory->version()), m_memory(memory), m_index(index) {}

  /// Store symbolic value to memory and propagate cast information
  /// This is intentionally a non-polymorphic override of the assignment
  /// operator in the superclass.
  ScalarVar<T>& operator=(const ScalarVar<T>& other) {
    ScalarVar<T>::set_cast(other.is_cast());
    m_memory->store_value(m_index, other.value());
    ScalarVar<T>::m_value = m_memory->load_value(m_index);
    ScalarVar<T>::m_version = m_memory->version();

    return *this;
  }

};

/// Pointer value to contiguous blocks of memory (e.g. an array)

/// The superclass represents the offset from the base of the allocated
/// symbolic memory region.
// TODO: Add stash/unstash for memory blocks (instead of pointer)
// TODO: Fix set_aggregate to handle pointer simplifications
template<typename T>
class Value<T*> : public ScalarValue<T*> {
private:
  // unstash() together with default assignment operator reset these fields
  T const * m_base;
  std::shared_ptr<Memory<T>> m_memory;

  // Total offset to memory block
  Value<size_t> index(const Value<size_t>& offset) const {
    const size_t current_index = ScalarValue<T*>::data() - m_base;
    Value<size_t> index(current_index + offset.data());
    Instr<ADD>::exec(*this, offset, index);
    return index;
  }

  Value(const T* data) = delete;
  Value(const T* data, const SharedExpr& expr) = delete;
  Value(const std::string& identifier) = delete;

public:
  Value(const std::shared_ptr<Memory<T>>& memory) : ScalarValue<T*>(0),
    m_memory(memory), m_base(0) {}

  Value(const Value& other) : ScalarValue<T*>(other),
    m_memory(other.m_memory), m_base(other.m_base) {}

  template<typename S>
  Value(const Value<S>&) = delete;

  MemoryBlock<T> operator[](size_t offset) const {
    return MemoryBlock<T>(m_memory, index(offset));
  }

  MemoryBlock<T> operator[](const Value<size_t>& offset) const {
    return MemoryBlock<T>(m_memory, index(offset));
  }

  MemoryBlock<T> operator[](const Var<size_t>& offset) const {
    return MemoryBlock<T>(m_memory, index(offset.value()));
  }
};

/// Pointer variable to contiguous blocks of memory (e.g. an array)

/// The superclass represents the offset from the base of the allocated
/// symbolic memory region.
template<typename T>
class Var<T*> : public ScalarVar<T*> {
private:

  Var(const T* data) = delete;

  template<typename S>
  Var(const Value<S>& value) = delete;

  template<typename S>
  Var(const Var<S>& other) = delete;

public:
  Var(const Value<T*>& value) : ScalarVar<T*>(value) {}
  Var(const Var& other) : ScalarVar<T*>(other) {}

  MemoryBlock<T> operator[](size_t offset) const {
    return ScalarVar<T*>::value()[offset];
  }

  MemoryBlock<T> operator[](const Value<size_t>& offset) const {
    return ScalarVar<T*>::value()[offset];
  }

  MemoryBlock<T> operator[](const Var<size_t>& offset) const {
    return ScalarVar<T*>::value()[offset];
  }
};

/// Allocate symbolic memory

/// Symbolic memory consists of @size symbolic memory blocks whose respective
/// size is `sizeof(T)`.
template<typename T>
Value<T*> malloc(size_t size, const std::string& identifier) {
  const std::shared_ptr<Memory<T>> memory(new Memory<T>(size, identifier));
  return Value<T*>(memory);
}

}

#endif /* MEMORY_H_ */
