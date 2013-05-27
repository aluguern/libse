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
class Zone {
public:
  static unsigned s_next_atom;
  static Zone s_bottom_element;

  const std::set<unsigned> m_atoms;

  // Bottom element
  Zone() : m_atoms() {}
  Zone(std::set<unsigned>&& atoms) : m_atoms(std::move(atoms)) {}

protected:
  /// Atom in the lattice
  Zone(unsigned atom) : m_atoms({atom}) {}

  template<typename T> friend class ZoneRelation;
  friend struct ZoneAtomSets;
  const std::set<unsigned>& atoms() const { return m_atoms; }

public:
  Zone(Zone&& other) : m_atoms(std::move(other.m_atoms)) {}
  Zone(const Zone& other) : m_atoms(other.m_atoms) {}

  /// \internal Reset the counter that make() uses
  static void reset(unsigned atom = 0) { s_next_atom = atom; }

  static Zone unique_atom() { return Zone(s_next_atom++); }
  static const Zone& bottom() { return s_bottom_element; }

  bool operator==(const Zone& other) const { return m_atoms == other.m_atoms; }
  bool operator!=(const Zone& other) const { return m_atoms != other.m_atoms; }
  bool is_bottom() const { return m_atoms.empty(); }

  /// Greatest lower bound
  Zone meet(const Zone& other) const {
    std::set<unsigned> intersection;
    std::set_intersection(m_atoms.cbegin(), m_atoms.cend(), other.m_atoms.cbegin(),
      other.m_atoms.cend(), std::inserter(intersection, intersection.begin()));
    return Zone(std::move(intersection));
  }

  /// Least upper bound
  Zone join(const Zone& other) const {
    std::set<unsigned> join_atoms(m_atoms);
    join_atoms.insert(other.m_atoms.cbegin(), other.m_atoms.cend());
    return Zone(std::move(join_atoms));
  }
};

}

#endif
