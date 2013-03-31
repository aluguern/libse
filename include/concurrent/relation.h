// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_RELATION_H_
#define LIBSE_CONCURRENT_RELATION_H_

#include <unordered_set>
#include <unordered_map>
#include <type_traits>
#include <functional>
#include <cstdint>
#include <memory>

#include "concurrent/event.h"

namespace se {

template<typename T>
class Predicate {
protected:
  Predicate() {}

public:
  virtual bool check(const T&) const = 0;
};

class ReadEventPredicate : public Predicate<std::shared_ptr<Event>> {
private:
  static ReadEventPredicate s_read_predicate;

  ReadEventPredicate() {}

public:
  virtual bool check(const std::shared_ptr<Event>& event_ptr) const {
    return event_ptr->is_read();
  }

  static const ReadEventPredicate& predicate() { return s_read_predicate; }
};

class WriteEventPredicate : public Predicate<std::shared_ptr<Event>> {
private:
  static WriteEventPredicate s_write_predicate;

  WriteEventPredicate() {}

public:
  virtual bool check(const std::shared_ptr<Event>& event_ptr) const {
    return event_ptr->is_write();
  }

  static const WriteEventPredicate& predicate() { return s_write_predicate; }
};

template<class T, class U, class THash = std::hash<T>, class UHash = std::hash<U>>
class Relation {
private:
  typedef std::unordered_multimap<T, U, THash> __Relation;
  __Relation m_relation;

public:
  /// Clears contents
  void clear() { m_relation.clear(); }

  void add(const T& a, const U& b) {
    m_relation.insert(typename __Relation::value_type(a, b));
  }

  /// Filter pairs based on key and predicate

  /// The output is stored in the last argument and is defined to be the set
  /// `{ b | (a, b) in R and predicate(b) = true}` where `R` is the set of
  /// pairs `T x U`.
  void find(const T& a, const Predicate<U>& predicate,
    std::unordered_set<U, UHash>& result) const {

    const std::pair<typename __Relation::const_iterator,
      typename __Relation::const_iterator> range(m_relation.equal_range(a));

    for (typename __Relation::const_iterator iter = range.first;
         iter != range.second; iter++) {

      if (predicate.check(iter->second)) {
        result.insert(iter->second);
      }
    }
  }

  /// Finds pairs for a given key and partitions them according to the predicate

  /// The output is stored in the last argument and is defined to be the pair
  /// of sets `({ b | (a, b) in R and predicate(b) = true}, { b | (a, b) in R
  /// and predicate(b) = false}` where `R` is the set of pairs `T x U`.
  void partition(const T& a, const Predicate<U>& predicate,
    std::pair</* true */ std::unordered_set<U, UHash>,
      /* false */ std::unordered_set<U, UHash>>& result) const {

    const std::pair<typename __Relation::const_iterator,
      typename __Relation::const_iterator> range(m_relation.equal_range(a));

    for (typename __Relation::const_iterator iter = range.first;
         iter != range.second; iter++) {

      if (predicate.check(iter->second)) {
        result.first.insert(iter->second);
      } else {
        result.second.insert(iter->second);
      }
    }
  }
};

/// \internal SingletonMemoryAddr::ptrs().size() is always exactly one
class SingletonMemoryAddr : public MemoryAddr {
  friend class SingletonMemoryAddrHash;
  template<typename T> friend class MemoryAddrRelation;

  SingletonMemoryAddr(uintptr_t ptr, bool is_shared) :
    MemoryAddr({ ptr }, is_shared) {}
};

/// \internal Hash a SingletonMemoryAddr
struct SingletonMemoryAddrHash {
  size_t operator()(const se::SingletonMemoryAddr& addr) const  {
    return *(addr.ptrs().cbegin());
  }
};

typedef std::unordered_set<SingletonMemoryAddr, SingletonMemoryAddrHash> MemoryAddrSet;

template<typename T = Event>
class MemoryAddrRelation {
static_assert(std::is_base_of<Event, T>::value, "T must be a subclass of Event");

private:
  std::unordered_set<std::shared_ptr<T>> m_event_ptrs;
  Relation<uintptr_t, std::shared_ptr<T>> m_relation;

  // Each memory address is guaranteed to have only one pointer
  MemoryAddrSet m_addrs;

public:
  MemoryAddrRelation() : m_event_ptrs(), m_relation(), m_addrs() {}

  /// Clears contents
  void clear() {
    m_event_ptrs.clear();
    m_addrs.clear();
    m_relation.clear();
  }

  /// All those events which were passed to relate()
  const std::unordered_set<std::shared_ptr<T>>& event_ptrs() const {
    return m_event_ptrs;
  }

  /// Set of related but opaque memory addresses
  const MemoryAddrSet addrs() const { return m_addrs; }

  void relate(const std::shared_ptr<T>& event_ptr) {
    m_event_ptrs.insert(event_ptr);

    const bool is_shared = event_ptr->addr().is_shared();
    for (uintptr_t ptr : event_ptr->addr().ptrs()) {
      m_addrs.insert(SingletonMemoryAddr(ptr, is_shared));
      m_relation.add(ptr, event_ptr);
    }
  }

  std::unordered_set<std::shared_ptr<T>> find(const MemoryAddr& addr,
    const Predicate<std::shared_ptr<T>>& predicate) const {

    std::unordered_set<std::shared_ptr<T>> result;
    for (uintptr_t ptr : addr.ptrs()) {
      m_relation.find(ptr, predicate, result);
    }
    return result;
  }

  /// Finds all read/write events that are associated with the given address
  std::pair<std::unordered_set<std::shared_ptr<T>>,
    std::unordered_set<std::shared_ptr<T>>>
  partition(const MemoryAddr& addr) const {

    typedef std::unordered_set<std::shared_ptr<T>> EventPtrSet;
    std::pair<EventPtrSet, EventPtrSet> result(std::make_pair(
      EventPtrSet(), EventPtrSet()));

    for (uintptr_t ptr : addr.ptrs()) {
      m_relation.partition(ptr, ReadEventPredicate::predicate(), result);
    }

    return result;
  }
};

}

#endif
