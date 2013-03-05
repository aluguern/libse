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

  MemoryAddr(std::unordered_set<uintptr_t>&& ptrs) :
    m_ptrs(std::move(ptrs)) {}

public:
  MemoryAddr(uintptr_t ptr) : m_ptrs{ ptr } {}

  const std::unordered_set<uintptr_t>& ptrs() const { return m_ptrs; }

  static MemoryAddr join(const MemoryAddr& a, const MemoryAddr& b) {
    std::unordered_set<uintptr_t> join_ptrs(a.m_ptrs);
    join_ptrs.insert(b.m_ptrs.cbegin(), b.m_ptrs.cend());
    return MemoryAddr(std::move(join_ptrs));
  }

  bool operator==(const MemoryAddr& other) const {
    return m_ptrs == other.m_ptrs;
  }
};

}

#endif
