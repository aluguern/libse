// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENCY_TAG_H_
#define LIBSE_CONCURRENCY_TAG_H_

#include <set>
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <utility>

namespace se {

/// An element in an atomistic lattice
class Tag {
private:
  static unsigned s_next_atom;
  static Tag s_bottom_element;

  const std::set<unsigned> m_atoms;

  // Bottom element
  Tag() : m_atoms() {}
  Tag(std::set<unsigned>&& atoms) : m_atoms(std::move(atoms)) {}

protected:
  /// Atom in the lattice
  Tag(unsigned atom) : m_atoms({atom}) {}

  template<typename T> friend class TagRelation;
  friend struct TagAtomSets;
  const std::set<unsigned>& atoms() const { return m_atoms; }

public:
  /// \internal Reset the counter that make() uses
  static void reset(unsigned atom = 0) { s_next_atom = atom; }

  static Tag unique_atom() { return Tag(s_next_atom++); }
  static Tag bottom() { return s_bottom_element; }

  bool operator==(const Tag& other) const { return m_atoms == other.m_atoms; }
  bool operator!=(const Tag& other) const { return m_atoms != other.m_atoms; }
  bool is_bottom() const { return m_atoms.empty(); }

  /// Greatest lower bound
  Tag meet(const Tag& other) const {
    std::set<unsigned> intersection;
    std::set_intersection(m_atoms.cbegin(), m_atoms.cend(), other.m_atoms.cbegin(),
      other.m_atoms.cend(), std::inserter(intersection, intersection.begin()));
    return Tag(std::move(intersection));
  }

  /// Least upper bound
  Tag join(const Tag& other) const {
    std::set<unsigned> join_atoms(m_atoms);
    join_atoms.insert(other.m_atoms.cbegin(), other.m_atoms.cend());
    return Tag(std::move(join_atoms));
  }
};

}

#endif
