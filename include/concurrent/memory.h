// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENCY_MEMORY_H_
#define LIBSE_CONCURRENCY_MEMORY_H_

#include <unordered_set>
#include <cstdint>
#include <cstddef>
#include <utility>

namespace se {

class MemoryAddr {
private:
  static uintptr_t s_next_addr;

  const std::unordered_set<uintptr_t> m_ptrs;
  const bool m_is_shared;

protected:
  MemoryAddr(std::unordered_set<uintptr_t>&& ptrs, bool is_shared) :
    m_ptrs(std::move(ptrs)), m_is_shared(is_shared) {}

  template<typename T> friend class MemoryAddrRelation;
  const std::unordered_set<uintptr_t>& ptrs() const { return m_ptrs; }

public:
  static void reset(uintptr_t addr = 0) { s_next_addr = addr; }

  bool is_shared() const { return m_is_shared; }

  bool operator==(const MemoryAddr& other) const {
    return m_is_shared == other.m_is_shared && m_ptrs == other.m_ptrs;
  }

  bool operator!=(const MemoryAddr& other) const { return !operator==(other); }

  MemoryAddr operator+(size_t offset) const {
    std::unordered_set<uintptr_t> ptrs;
    for (uintptr_t ptr : m_ptrs) { ptrs.insert(ptr + offset); }
    return MemoryAddr(std::move(ptrs), m_is_shared);
  }

  static MemoryAddr join(const MemoryAddr& a, const MemoryAddr& b) {
    std::unordered_set<uintptr_t> join_ptrs(a.m_ptrs);
    join_ptrs.insert(b.m_ptrs.cbegin(), b.m_ptrs.cend());
    return MemoryAddr(std::move(join_ptrs), a.is_shared() || b.is_shared());
  }

  /// Identifier for a specific or range of memory cells

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
    std::unordered_set<uintptr_t> ptrs;
    for (; 0 < size; size--) {
      ptrs.insert(s_next_addr);
      s_next_addr += sizeof(T);
    }

    return MemoryAddr(std::move(ptrs), is_shared);
  }
};

}

#endif
