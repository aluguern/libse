// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_SLICER_H_
#define LIBSE_CONCURRENT_SLICER_H_

#include <map>
#include <stack>

#include "concurrent/thread.h"

namespace se {

/// Program location
typedef unsigned Location;

/// Slice every path in the series-parallel DAG
constexpr unsigned MAX_SLICE_FREQ = (1u << 10);

/// Renders a concurrent program as a set of series-parallel DAGs


/// The output of a Slicer is a set (i.e. forest) of directed, acyclic
/// series-parallel graphs (DAGs) whose vertices are \ref Block "blocks".
/// To automate the construction of these blocks, the source code of a
/// concurrent program `P` would typically have to be transformed such
/// that the appropriate Slicer member functions are called at each
/// control flow point in `P`.
///
/// Example:
///
///      if (c == '?') {
///        c = 'A';
///      } else {
///        c = 'B';
///      }
///
/// should be turned into
///
///      Slicer slicer;
///      if (slicer.begin_then(__COUNTER__, c == '?')) {
///        c = 'A';
///      }
///      if (slicer.begin_else(__COUNTER__)) {
///        c = 'B';
///      }
///      slicer.end_branch();
///
/// Note that this source-to-source transformation requires the immediate
/// post-dominator of every control point in the program's control-flow
/// graph to be computable. In general, this is impossible when there
/// are goto statements that can jump to arbitrary program locations.
class Slicer {
private:
  struct Branch {
    bool execute;
    bool flip;
  };

  const unsigned m_slice_freq;
  typedef std::map<Location, Branch> BranchMap;
  BranchMap m_branch_map;
  unsigned m_slice_count;
  std::stack<bool> m_branch_execute_stack;

public:
  /// If the argument is zero, the series-parallel DAG is never sliced
  Slicer(unsigned slice_freq = 0) :
    m_slice_freq(slice_freq),
    m_branch_map(),
    m_slice_count(1),
    m_branch_execute_stack() {}

  /// Number of slices made
  unsigned slice_count() const {
    return m_slice_count;
  }

  void begin_slice_loop() {
    Threads::begin_slice_loop();
  }

  /// Begin conditional block

  /// Execute the conditional block if and only if the return value is true.
  /// This member function must be called exactly once prior to calling
  /// end_branch(Location).
  ///
  /// \returns execute the conditional block?
  bool begin_then_branch(Location loc, std::shared_ptr<ReadInstr<bool>> condition_ptr) {
    assert(nullptr != condition_ptr);

    ThisThread::begin_then(condition_ptr);

    if (m_slice_freq == 0) {
      return true;
    }

    bool execute = false;
    const BranchMap::iterator branch_it(m_branch_map.find(loc));
    if (branch_it == m_branch_map.cend()) {
      const Branch new_branch = {execute, false};
      m_branch_map.insert(BranchMap::value_type(loc, new_branch));
    } else {
      execute = branch_it->second.execute;
    }

    m_branch_execute_stack.push(execute);
    return execute;
  }

  /// Begin optional block

  /// A call of this function demarcates the end of the "then" block and the
  /// beginning of the "else" block. Therefore, begin_else() can only be called
  /// after calling begin_then() and then only once. Execute the "else" block
  /// if and only if the return value is true.
  ///
  /// \return execute the optional block?
  bool begin_else_branch(Location loc) {
    ThisThread::begin_else();

    if (m_slice_freq == 0) {
      return true;
    }

    return !m_branch_execute_stack.top();
  }

  /// Demarcate the end of a conditional "then" and an optional "else" branch

  /// end_branch() must always be called exactly once such that its call site
  /// is the immediate post-dominator of begin_then().
  void end_branch(Location loc) {
    ThisThread::end_branch();

    if (m_slice_freq > 0) {
      m_branch_execute_stack.pop();
    }
  }

  /// Look for another slice to analyze

  /// \returns is there another slice to analyze?
  bool next_slice() {
    if (m_branch_map.empty()) {
      return false;
    }

    BranchMap::reverse_iterator rev_it(m_branch_map.rbegin());
    while (rev_it != m_branch_map.rend() && rev_it->second.flip) {
      // as we flip higher up branches we want to revisit
      // both directions of any lower branches
      rev_it->second.flip = false;
      rev_it++;
    }

    if (rev_it == m_branch_map.rend()) {
      return false;
    }

    rev_it->second.flip = true;
    rev_it->second.execute = !rev_it->second.execute;

    m_slice_count++;
    return true;
  }
};

}

#endif
