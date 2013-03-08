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

protected:
  void add(const T& a, const U& b) {
    m_relation.insert(typename __Relation::value_type(a, b));
  }

public:
  virtual ~Relation() {}

  /// Filter relation pairs

  /// \returns set `{ b | (a, b) in R and p(b) is true}` where `R` is
  /// the set of pairs `T x U`
  std::unordered_set<U, UHash> find(const T& a, const Predicate<U>& p) const {
    const std::pair<typename __Relation::const_iterator,
      typename __Relation::const_iterator> range(m_relation.equal_range(a));

    std::unordered_set<U, UHash> set;
    for (typename __Relation::const_iterator iter = range.first;
         iter != range.second; iter++) {

      if (p.check(iter->second)) {
        set.insert(iter->second);
      }
    }

    return set;
  }
};

template<typename T = Event>
class MemoryAccessRelation : public Relation<uintptr_t, std::shared_ptr<T>> {
static_assert(std::is_base_of<Event, T>::value, "T must be a subclass of Event");

private:
  typedef Relation<uintptr_t, std::shared_ptr<T>> Super;

public:
  void relate(const std::shared_ptr<T>& event_ptr) {
    for (uintptr_t ptr : event_ptr->addr().ptrs()) {
      Super::add(ptr, event_ptr);
    }
  }
};

}

#endif
