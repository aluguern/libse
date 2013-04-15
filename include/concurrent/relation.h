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

/// \internal Atom in the Tag lattice
class TagAtom : public Tag {
  friend class TagAtomHash;
  friend class TagAtomSets;
  template<typename T> friend class TagRelation;

  TagAtom(unsigned atom) : Tag(atom) {}

public:
  operator unsigned() const { return *atoms().cbegin(); }
};

/// \internal Hash value of a TagAtom
struct TagAtomHash {
  size_t operator()(const se::TagAtom& tag_atom) const  {
    return static_cast<unsigned>(tag_atom);
  }
};

typedef std::unordered_set<TagAtom, TagAtomHash> TagAtomSet;

/// Helper for sets of \ref TagAtom atoms
struct TagAtomSets {
  static TagAtomSet tag_atom_set(const Tag& tag) {
    TagAtomSet tag_atoms;
    for (unsigned atom : tag.atoms()) {
      tag_atoms.insert(TagAtom(atom));
    }

    return std::move(tag_atoms);
  }
};

template<typename T = Event>
class TagRelation {
static_assert(std::is_base_of<Event, T>::value, "T must be a subclass of Event");

private:
  std::unordered_set<std::shared_ptr<T>> m_event_ptrs;
  Relation<unsigned, std::shared_ptr<T>> m_relation;
  TagAtomSet m_tag_atoms;

public:
  TagRelation() : m_event_ptrs(), m_relation(), m_tag_atoms() {}

  /// Clears contents
  void clear() {
    m_event_ptrs.clear();
    m_tag_atoms.clear();
    m_relation.clear();
  }

  /// All those events that were passed to relate(const std::shared_ptr<T>&)
  const std::unordered_set<std::shared_ptr<T>>& event_ptrs() const {
    return m_event_ptrs;
  }

  const TagAtomSet tag_atoms() const { return m_tag_atoms; }

  void relate(const std::shared_ptr<T>& event_ptr) {
    assert(!event_ptr->tag().is_bottom());

    m_event_ptrs.insert(event_ptr);

    for (unsigned atom : event_ptr->tag().atoms()) {
      m_tag_atoms.insert(TagAtom(atom));
      m_relation.add(atom, event_ptr);
    }
  }

  std::unordered_set<std::shared_ptr<T>> find(const Tag& tag,
    const Predicate<std::shared_ptr<T>>& predicate) const {

    std::unordered_set<std::shared_ptr<T>> result;
    for (unsigned atom : tag.atoms()) {
      m_relation.find(atom, predicate, result);
    }
    return result;
  }

  /// Finds all read/write events that are associated with the given tag
  std::pair<std::unordered_set<std::shared_ptr<T>>,
    std::unordered_set<std::shared_ptr<T>>> partition(const Tag& tag) const {

    typedef std::unordered_set<std::shared_ptr<T>> EventPtrSet;
    std::pair<EventPtrSet, EventPtrSet> result(std::make_pair(
      EventPtrSet(), EventPtrSet()));

    for (unsigned atom : tag.atoms()) {
      m_relation.partition(atom, ReadEventPredicate::predicate(), result);
    }

    return result;
  }
};

}

#endif
