// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENCY_MEMORY_H_
#define LIBSE_CONCURRENCY_MEMORY_H_

#include <set>
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <utility>

namespace se {

/// An element in a lattice

/// The top element in the lattice represents all possible memory addresses.
/// Dually, bottom is the empty set of memory addresses. Thus, when an event
/// is associated with the bottom element, it can never affect any memory.
class MemoryAddr {
private:
  static uintptr_t s_next_addr;

  const std::set<uintptr_t> m_ptrs;
  const bool m_is_shared;

protected:
  MemoryAddr(std::set<uintptr_t>&& ptrs, bool is_shared) :
    m_ptrs(std::move(ptrs)), m_is_shared(is_shared) {}

  template<typename T> friend class MemoryAddrRelation;
  const std::set<uintptr_t>& ptrs() const { return m_ptrs; }

public:
  static void reset(uintptr_t addr = 0) { s_next_addr = addr; }

  bool is_shared() const { return m_is_shared; }

  bool operator==(const MemoryAddr& other) const {
    return m_is_shared == other.m_is_shared && m_ptrs == other.m_ptrs;
  }

  bool operator!=(const MemoryAddr& other) const { return !operator==(other); }

  bool is_bottom() const { return m_ptrs.empty(); }

  MemoryAddr meet(const MemoryAddr& other) const {
    std::set<uintptr_t> intersection;
    std::set_intersection(m_ptrs.cbegin(), m_ptrs.cend(), other.m_ptrs.cbegin(),
      other.m_ptrs.cend(), std::inserter(intersection, intersection.begin()));
    return MemoryAddr(std::move(intersection), m_is_shared && other.m_is_shared);
  }

  MemoryAddr join(const MemoryAddr& other) const {
    std::set<uintptr_t> join_ptrs(m_ptrs);
    join_ptrs.insert(other.m_ptrs.cbegin(), other.m_ptrs.cend());
    return MemoryAddr(std::move(join_ptrs), m_is_shared || other.m_is_shared);
  }

  MemoryAddr operator+(size_t offset) const {
    std::set<uintptr_t> ptrs;
    for (uintptr_t ptr : m_ptrs) { ptrs.insert(ptr + offset); }
    return MemoryAddr(std::move(ptrs), m_is_shared);
  }

  /// Element in a lattice for a single or multiple memory cells

  /// \param is_shared - can other threads modify the memory?
  /// \param size - identify the next `sizeof(T)` bytes
  ///
  /// The return value identifies a single memory cell if and only if
  /// `size == 1`. That is, `alloc<U>(_, 1)` always acts as a base address
  /// for `sizeof(U)` bytes. Thus, `alloc<T>(_, N)` for `1 < N` is not the
  /// same as alloc<T[N]>(_, 1). While both allocate the same number of
  /// memory cells, the former identifies all `N` of these whereas the
  /// latter identifies only the first one.
  template<typename T>
  static MemoryAddr alloc(bool is_shared = true, size_t size = 1) {
    std::set<uintptr_t> ptrs;
    for (; 0 < size; size--) {
      ptrs.insert(s_next_addr);
      s_next_addr += sizeof(T);
    }

    return MemoryAddr(std::move(ptrs), is_shared);
  }
};

}

#endif
