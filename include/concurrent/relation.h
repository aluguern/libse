// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_RELATION_H_
#define LIBSE_CONCURRENT_RELATION_H_

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
  virtual ~Relation() {}

  void add(const T& a, const U& b) {
    m_relation.insert(typename __Relation::value_type(a, b));
  }

  /// Filter relation pairs

  /// The output is stored in the last argument and is defined to be the set
  /// `{ b | (a, b) in R and p(b) is true}` where `R` is the set of pairs `T x U`.
  void find(const T& a, const Predicate<U>& p, std::unordered_set<U, UHash>& result) const {
    const std::pair<typename __Relation::const_iterator,
      typename __Relation::const_iterator> range(m_relation.equal_range(a));

    for (typename __Relation::const_iterator iter = range.first;
         iter != range.second; iter++) {

      if (p.check(iter->second)) {
        result.insert(iter->second);
      }
    }
  }
};

template<typename T = Event>
class MemoryAddrRelation {
static_assert(std::is_base_of<Event, T>::value, "T must be a subclass of Event");

private:
  Relation<uintptr_t, std::shared_ptr<T>> relation;

public:
  void relate(const std::shared_ptr<T>& event_ptr) {
    for (uintptr_t ptr : event_ptr->addr().ptrs()) {
      relation.add(ptr, event_ptr);
    }
  }

  std::unordered_set<std::shared_ptr<T>> find(const MemoryAddr& addr,
    const Predicate<std::shared_ptr<T>>& predicate) {

    std::unordered_set<std::shared_ptr<T>> result;
    for (uintptr_t ptr : addr.ptrs()) {
      relation.find(ptr, predicate, result);
    }
    return result;
  }
};

}

#endif
