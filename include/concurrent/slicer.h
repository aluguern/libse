// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_EXPLORER_H_
#define LIBSE_CONCURRENT_EXPLORER_H_

#include <map>

#include "concurrent/thread.h"

namespace se {

/// Program location
typedef unsigned Location;

/// Slices a concurrent program

/// The slicer finds the events and path conditions along a chain of blocks in
/// the concurrent program under scrutiny. This requires the source code of the
/// concurrent program to be transformed such that the appropriate Slicer member
/// functions are called at each control flow point in the program.
//
/// Example:
///
///      if (c == '?') {
///        c = 'A';
///      } else {
///        c = 'B';
///      }
///
/// turns into
///
///      Explorer explorer;
///      if (explorer.begin_block(__COUNTER__, c == '?')) {
///        c = 'A';
///      } else {
///        c = 'B';
///      }
///      explorer.end_block();
///
/// Note that this source-to-source transformation requires the immediate
/// post-dominator of every control point in the program's control-flow graph
/// to be always computable. In general, this is not the case in unstructured
/// programs where goto statements can jump to an arbitrary program location.
class Slicer {
private:
  static std::unique_ptr<ReadInstr<bool>> negate(
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr) {

    return std::unique_ptr<ReadInstr<bool>>(new UnaryReadInstr<NOT, bool>(
      condition_ptr));
  }

  struct Branch {
    std::shared_ptr<ReadInstr<bool>> condition_ptr;
    bool direction;
    bool flip_flag;
  };

  typedef std::map<Location, Branch> BranchMap;
  BranchMap m_branch_map;
  unsigned m_slice_count;

public:
  Slicer() :
    m_branch_map(),
    m_slice_count(1) {}

  /// Number of slices made
  unsigned slice_count() const {
    return m_slice_count;
  }

  void begin_slice_loop() {
    Threads::begin_slice_loop();
  }

  /// Record a Boolean branch instruction at a certain program location

  /// This member function must be called exactly once prior to calling
  /// end_block(Location).
  ///
  /// \returns execute the conditional block?
  bool begin_block(Location loc, std::shared_ptr<ReadInstr<bool>> condition_ptr) {
    assert(nullptr != condition_ptr);

    bool direction = false;
    const BranchMap::iterator branch_it(m_branch_map.find(loc));
    if (branch_it == m_branch_map.cend()) {
      const Branch new_branch = {condition_ptr, direction, false};
      m_branch_map.insert(BranchMap::value_type(loc, new_branch));
    } else {
      direction = branch_it->second.direction;
      branch_it->second.condition_ptr = condition_ptr;
    }

    if (direction) {
      ThisThread::begin_block(condition_ptr);
    } else {
      ThisThread::begin_block(negate(condition_ptr));
    }
    return direction;
  }

  /// Demarcate the end of a block

  /// end_block() must always be called exactly once such that its call site
  /// is the immediate post-dominator of begin_block().
  void end_block(Location loc) {
    ThisThread::end_block();
  }

  /// Look for another slice to analyze

  /// \returns is there another slice to analyze?
  bool next_slice() {
    if (m_branch_map.empty()) {
      return false;
    }

    BranchMap::reverse_iterator rev_it(m_branch_map.rbegin());
    while (rev_it != m_branch_map.rend() && rev_it->second.flip_flag) {
      // as we flip higher up branches we want to revisit
      // both directions of any lower branches
      rev_it->second.flip_flag = false;
      rev_it++;
    }

    if (rev_it == m_branch_map.rend()) {
      return false;
    }

    rev_it->second.flip_flag = true;
    rev_it->second.direction = !rev_it->second.direction;

    m_slice_count++;
    return true;
  }
};

}

#endif
