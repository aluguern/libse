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

  MemoryAddr(std::unordered_set<uintptr_t>&& ptrs, bool is_shared) :
    m_ptrs(std::move(ptrs)), m_is_shared(is_shared) {}

  MemoryAddr(uintptr_t ptr, bool is_shared) :
    m_ptrs{ ptr }, m_is_shared(is_shared) {}

  template<typename T> friend class MemoryAddrRelation;
  friend class SingletonMemoryAddrHash;
  const std::unordered_set<uintptr_t>& ptrs() const { return m_ptrs; }

public:
  static void reset(uintptr_t addr = 0) { s_next_addr = addr; }

  bool is_shared() const { return m_is_shared; }

  bool operator==(const MemoryAddr& other) const {
    return m_ptrs == other.m_ptrs && m_is_shared == other.m_is_shared;
  }

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

  template<typename T>
  static MemoryAddr alloc(bool is_shared = true, size_t size = 1) {
    MemoryAddr addr(s_next_addr, is_shared);
    s_next_addr += sizeof(T) * size;
    return std::move(addr);
  }
};

}

#endif
