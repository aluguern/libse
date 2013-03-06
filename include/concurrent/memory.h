// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENCY_MEMORY_H_
#define LIBSE_CONCURRENCY_MEMORY_H_

#include <unordered_set>
#include <cstdint>
#include <utility>

namespace se {

class MemoryAddr {
private:
  const std::unordered_set<uintptr_t> m_ptrs;
  const bool m_is_shared;

  MemoryAddr(std::unordered_set<uintptr_t>&& ptrs, bool is_shared) :
    m_ptrs(std::move(ptrs)), m_is_shared(is_shared) {}

public:
  MemoryAddr(uintptr_t ptr, bool is_shared = true) :
    m_ptrs{ ptr }, m_is_shared(is_shared) {}

  const std::unordered_set<uintptr_t>& ptrs() const { return m_ptrs; }

  static MemoryAddr join(const MemoryAddr& a, const MemoryAddr& b) {
    std::unordered_set<uintptr_t> join_ptrs(a.m_ptrs);
    join_ptrs.insert(b.m_ptrs.cbegin(), b.m_ptrs.cend());
    return MemoryAddr(std::move(join_ptrs), a.is_shared() || b.is_shared());
  }

  bool is_shared() const { return m_is_shared; }

  bool operator==(const MemoryAddr& other) const {
    return m_ptrs == other.m_ptrs && m_is_shared == other.m_is_shared;
  }
};

}

#endif
