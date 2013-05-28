// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_SLICE_H_
#define LIBSE_CONCURRENT_SLICE_H_

#include <stack>

#include "concurrent/instr.h"
#include "concurrent/block.h"
#include "concurrent/encoder_c0.h"

namespace se {

typedef std::shared_ptr<Event> EventPtr;

/// Bounded loop unwinding policy

/// Two loop policies with the same \ref LoopPolicy::id() "identifier" must
/// have identical \ref LoopPolicy::unwinding_bound() "loop unwinding bounds".
class LoopPolicy {
private:
  const unsigned m_id;
  const unsigned m_unwinding_bound;

  template<unsigned id, unsigned unwinding_bound>
  friend constexpr LoopPolicy make_loop_policy();

  /// \pre 0 < bound
  constexpr LoopPolicy(const unsigned id, const unsigned bound) :
    m_id(id), m_unwinding_bound(bound) {}

public:
  constexpr LoopPolicy(const LoopPolicy& other) :
    m_id(other.m_id), m_unwinding_bound(other.m_unwinding_bound) {}

  constexpr unsigned id() const { return m_id; }
  constexpr unsigned unwinding_bound() const { return m_unwinding_bound; }
};

/// \pre if id == id', then unwinding_bound == unwinding_bound'
template<unsigned id, unsigned unwinding_bound>
constexpr LoopPolicy make_loop_policy() {
  static_assert(0 < unwinding_bound, "Loop unwinding bound must be positive");
  return LoopPolicy(id, unwinding_bound);
}

class Loop {
private:
  const LoopPolicy m_policy;
  unsigned m_unwinding_counter;

public:
  constexpr Loop(const LoopPolicy& policy) :
    m_policy(policy), m_unwinding_counter(policy.unwinding_bound()) {}

  constexpr Loop(Loop&& other) :
    m_policy(other.m_policy),
    m_unwinding_counter(other.m_unwinding_counter) {}

  Loop(const Loop&) = delete;

  constexpr unsigned policy_id() const {
    return m_policy.id();
  }

  constexpr unsigned unwinding_bound() const {
    return m_policy.unwinding_bound();
  }

  unsigned unwinding_counter() const {
    return m_unwinding_counter;
  }

  /// \pre 0 < unwinding_counter()
  void decrement_unwinding_counter() {
    assert(0 < m_unwinding_counter);
    m_unwinding_counter--;
  }
};

/// Series-parallel (sub)graph
class Slice {
private:
  const std::shared_ptr<Block> m_most_outer_block_ptr;

  std::shared_ptr<Block> m_current_block_ptr;

  // Each structurally nested loop in program is pushed onto this stack. This
  // requires an inner loop to be fully unwound before a loop containing it.
  std::stack<Loop> m_loop_stack;

  void set_current_block_ptr(const std::shared_ptr<Block>& block_ptr) {
    m_current_block_ptr = block_ptr;
  }

public:
  Slice() :
    m_most_outer_block_ptr(Block::make_root()),
    m_current_block_ptr(new Block(m_most_outer_block_ptr)),
    m_loop_stack(/* empty */) {

    m_most_outer_block_ptr->push_inner_block_ptr(m_current_block_ptr);
  }

  Slice(const Slice&) = delete;

  Slice(Slice&& other) :
    m_most_outer_block_ptr(std::move(other.m_most_outer_block_ptr)),
    m_current_block_ptr(std::move(other.m_current_block_ptr)),
    m_loop_stack(std::move(other.m_loop_stack)) {}

  /// Append event to the current block's event list
  void append(const std::shared_ptr<Event>& event_ptr) {
    m_current_block_ptr->append(event_ptr);
  }

  /// Append all read events that are in the given instruction
  template<typename T>
  void append_all(const ReadInstr<T>& instr) {
    std::forward_list<std::shared_ptr<Event>> read_event_ptrs;
    instr.filter(read_event_ptrs);

    append_all(read_event_ptrs);
  }

  /// Append all the given event pointers
  void append_all(const std::forward_list<std::shared_ptr<Event>>& event_ptrs) {
    m_current_block_ptr->append_all(event_ptrs);
  }

  /// Root of the series-parallel graph
  
  /// The most outer block has an empty body but at least one inner block.
  /// But the most outer block has never an else block.
  std::shared_ptr<Block> most_outer_block_ptr() const {
    assert(m_most_outer_block_ptr->body().empty());
    assert(!m_most_outer_block_ptr->inner_block_ptrs().empty());
    assert(nullptr == m_most_outer_block_ptr->else_block_ptr());

    return m_most_outer_block_ptr;
  }

  const Block& current_block_ref() const {
    return *m_current_block_ptr;
  }

  std::shared_ptr<Block> current_block_ptr() const {
    return m_current_block_ptr;
  }

  std::forward_list<std::shared_ptr<Event>>& current_block_body() const {
    return m_current_block_ptr->m_body;
  }

  /// Unwind the loop once more if the loop unwinding policy permits it

  /// If the return value is false, the effect of a subsequent call to
  /// the function is undefined.
  ///
  /// \return continue loop unwinding?
  bool unwind_loop(std::shared_ptr<ReadInstr<bool>> condition_ptr,
    const LoopPolicy& policy) {

    assert(nullptr != condition_ptr);

    if (m_loop_stack.empty() || m_loop_stack.top().policy_id() != policy.id()) {
      m_loop_stack.push(Loop(policy));
    }

    assert(!m_loop_stack.empty());
    assert(m_loop_stack.top().policy_id() == policy.id());
    assert(m_loop_stack.top().unwinding_bound() == policy.unwinding_bound());

    Loop& current_loop = m_loop_stack.top();
    bool continue_unwinding = true;
    if (0 < current_loop.unwinding_counter()) {
      current_loop.decrement_unwinding_counter();
      begin_then(condition_ptr);
    } else {
      // close all unwound branches of the current loop
      for (unsigned k = 0; k < current_loop.unwinding_bound(); k++) {
        end_branch();
      }

      m_loop_stack.pop();
      continue_unwinding = false;
    }

    return continue_unwinding;
  }

  /// Begin conditional block
  ///
  /// This member function must be called exactly once prior to calling
  /// begin_else() or end_branch().
  void begin_then(std::shared_ptr<ReadInstr<bool>> condition_ptr) {
    assert(nullptr != condition_ptr);
    append_all(*condition_ptr);

    if (m_current_block_ptr->condition_ptr()) {
      // start nested branch inside current conditional block
      const std::shared_ptr<Block> then_block_ptr(new Block(
        m_current_block_ptr, condition_ptr));

      m_current_block_ptr->push_inner_block_ptr(then_block_ptr);
      set_current_block_ptr(then_block_ptr);
    } else {
      // unconditional blocks cannot have inner blocks
      assert(m_current_block_ptr->inner_block_ptrs().empty());

      if (m_current_block_ptr->body().empty()) {
        // reuse current unconditional and empty block
        m_current_block_ptr->m_condition_ptr = condition_ptr;
      } else {
        const std::shared_ptr<Block> outer_block_ptr(
          m_current_block_ptr->outer_block_ptr());

        // next branch after an unconditional block inside outer block
        std::shared_ptr<Block> then_block_ptr(new Block(outer_block_ptr,
          std::move(condition_ptr)));

        outer_block_ptr->push_inner_block_ptr(then_block_ptr);
        set_current_block_ptr(then_block_ptr);
      }
    }
  }

  /// Begin optional block

  /// A call of this function demarcates the end of the "then" block and the
  /// beginning of the "else" block. Therefore, begin_else() can only be called
  /// after calling begin_then() and then only once.
  void begin_else() {
    if (!m_current_block_ptr->condition_ptr()) {
      // unconditional blocks cannot have inner blocks
      assert(m_current_block_ptr->inner_block_ptrs().empty());

      if (m_current_block_ptr->body().empty()) {
        set_current_block_ptr(m_current_block_ptr->outer_block_ptr());

        // delete last empty inner block inside "then" branch
        m_current_block_ptr->pop_inner_block_ptr();
      } else {
        set_current_block_ptr(m_current_block_ptr->outer_block_ptr());
      }
    }

    // we're now in the block for the "then" branch
    assert(nullptr != m_current_block_ptr->outer_block_ptr());
    std::shared_ptr<Block> else_block_ptr(new Block(
      m_current_block_ptr->outer_block_ptr(),
      Bools::negate(m_current_block_ptr->condition_ptr())));

    m_current_block_ptr->m_else_block_ptr = else_block_ptr;
    set_current_block_ptr(else_block_ptr);
  }

  /// Create next nested, unconditional block inside outer block

  /// end_branch() must always be called exactly once such that its call site is
  /// the immediate post-dominator of begin_then().
  void end_branch() {
    std::shared_ptr<Block> outer_block_ptr(
      m_current_block_ptr->outer_block_ptr());

    if (!m_current_block_ptr->condition_ptr()) {
      // unconditional blocks cannot have inner blocks
      assert(m_current_block_ptr->inner_block_ptrs().empty());

      if (m_current_block_ptr->body().empty()) {
        // delete last empty inner block
        outer_block_ptr->pop_inner_block_ptr();
      }

      outer_block_ptr = outer_block_ptr->outer_block_ptr();
      assert(nullptr != outer_block_ptr);
    }

    // next block is initially unconditional and empty
    std::shared_ptr<Block> next_block_ptr(new Block(outer_block_ptr));
    outer_block_ptr->push_inner_block_ptr(next_block_ptr);
    set_current_block_ptr(next_block_ptr);
  }
};

}

#endif
