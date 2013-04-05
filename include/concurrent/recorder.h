// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_RECORDER_H_
#define LIBSE_CONCURRENT_RECORDER_H_

#include <stack>

#include "concurrent/instr.h"
#include "concurrent/block.h"
#include "concurrent/encoder.h"

namespace se {

class Recorder;
namespace ThisThread {
  Recorder& recorder();
  unsigned thread_id();
}

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
    m_policy(other.m_policy), m_unwinding_counter(other.m_unwinding_counter) {}

  Loop(const Loop&) = delete;

  constexpr unsigned policy_id() const { return m_policy.id(); }
  constexpr unsigned unwinding_bound() const { return m_policy.unwinding_bound(); }

  unsigned unwinding_counter() const { return m_unwinding_counter; }

  /// \pre 0 < unwinding_counter()
  void decrement_unwinding_counter() {
    assert(0 < m_unwinding_counter);
    m_unwinding_counter--;
  }
};

/// Records events and path conditions on a per-thread basis

/// The output of a recorder is a series-parallel graph of \ref Block "blocks".
/// To automate the construction of these blocks, the source code of a program
/// would typically have to be transformed such that the appropriate Recorder
/// member functions are called at each control flow point in the program.
//
/// Example:
///
///      if (c == '?') {
///        c = 'A';
///      } else {
///        c = 'B';
///      }
/// turns into
///      if (recorder.begin_then(c == '?')) {
///        c = 'A';
///      }
///      if (recorder.begin_else()) {
///        c = 'B';
///      }
///      recorder.end_branch();
///
/// Note that this source-to-source transformation requires the immediate
/// post-dominator of every control point in the program's control-flow graph
/// to be always computable. In particular, this is not the case in unstructured
/// programs where goto statements can jump to an arbitrary program location.
class Recorder {
private:
  static const std::shared_ptr<ReadInstr<bool>> s_true_condition_ptr;

  const unsigned m_thread_id;
  const std::shared_ptr<Block> m_most_outer_block_ptr;

  std::shared_ptr<Block> m_current_block_ptr;
  std::shared_ptr<ReadInstr<bool>> m_current_block_condition_ptr;

  // Each structurally nested loop in program is pushed onto this stack. This
  // requires an inner loop to be fully unwound before a loop containing it.
  std::stack<Loop> m_loop_stack;

  static std::unique_ptr<ReadInstr<bool>> negate(
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr) {

    return std::unique_ptr<ReadInstr<bool>>(new UnaryReadInstr<NOT, bool>(
      condition_ptr));
  }

  void set_current_block_ptr(const std::shared_ptr<Block>& block_ptr) {
    m_current_block_ptr = block_ptr;
    m_current_block_condition_ptr = nullptr;
  }

public:
  Recorder(unsigned thread_id) : m_thread_id(thread_id),
    m_most_outer_block_ptr(Block::make_root()),
    m_current_block_ptr(std::unique_ptr<Block>(new Block(
      m_most_outer_block_ptr))), m_current_block_condition_ptr(),
      m_loop_stack(/* empty */) {

    m_most_outer_block_ptr->push_inner_block_ptr(m_current_block_ptr);
  }

  Recorder(const Recorder&) = delete;

  Recorder(Recorder&& other) : m_thread_id(other.m_thread_id),
    m_most_outer_block_ptr(std::move(other.m_most_outer_block_ptr)),
    m_current_block_ptr(std::move(other.m_current_block_ptr)),
    m_current_block_condition_ptr(other.m_current_block_condition_ptr),
    m_loop_stack(std::move(other.m_loop_stack)) {}

  unsigned thread_id() const { return m_thread_id; }

  /// Insert in the body all those read events that are in the given instruction
  template<typename T>
  void insert_all(const ReadInstr<T>& instr) {
    std::forward_list<std::shared_ptr<Event>> read_event_ptrs;
    instr.filter(read_event_ptrs);

    m_current_block_ptr->insert_all(read_event_ptrs);
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

  /// Conjunction of nested block conditions
  std::shared_ptr<ReadInstr<bool>> block_condition_ptr() {
    // TODO: Use a stack and restore previously computed block conditions
    if (m_current_block_condition_ptr) {
      return m_current_block_condition_ptr;
    }

    std::forward_list<std::shared_ptr<ReadInstr<bool>>> condition_ptrs;
    size_t nary_size = 0;
    for (std::shared_ptr<Block> block_ptr(m_current_block_ptr->outer_block_ptr());
         block_ptr != m_most_outer_block_ptr;
         block_ptr = block_ptr->outer_block_ptr(), nary_size++) {

      assert(nullptr != block_ptr->condition_ptr());
      condition_ptrs.push_front(block_ptr->condition_ptr());
    }

    if (condition_ptrs.empty()) {
      return m_current_block_ptr->condition_ptr();
    }

    if (m_current_block_ptr->condition_ptr()) {
      nary_size++;
      condition_ptrs.push_front(m_current_block_ptr->condition_ptr());
    } else if (nary_size == 1) {
      return condition_ptrs.front();
    }

    m_current_block_condition_ptr = std::unique_ptr<ReadInstr<bool>>(
      new NaryReadInstr<LAND, bool>(std::move(condition_ptrs), nary_size));
    return m_current_block_condition_ptr;
  }

  const Block& current_block_ref() const { return *m_current_block_ptr; }
  std::shared_ptr<Block> current_block_ptr() const { return m_current_block_ptr; }

  /// \internal
  std::forward_list<std::shared_ptr<Event>>& current_block_body() const {
    return m_current_block_ptr->m_body;
  }

  /// Unwind the loop once more if the loop unwinding policy permits it

  /// If the return value is false, the effect of a subsequent call to
  /// the function is undefined.
  ///
  /// \return continue loop unwinding?
  bool unwind_loop(std::unique_ptr<ReadInstr<bool>> condition_ptr,
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
      continue_unwinding = begin_then(std::move(condition_ptr));
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

  /// Annotation for a conditional jump instruction
  ///
  /// The inner block is executed if and only if the return value is true.
  /// This member function must be called exactly once prior to calling
  /// begin_else() or end_branch().
  ///
  /// \return execute "then" block?
  bool begin_then(std::unique_ptr<ReadInstr<bool>> condition_ptr) {
    assert(nullptr != condition_ptr);
    insert_all(*condition_ptr);

    if (m_current_block_ptr->condition_ptr()) {
      // start nested branch inside current conditional block
      const std::shared_ptr<Block> then_block_ptr(new Block(
        m_current_block_ptr, std::move(condition_ptr)));

      m_current_block_ptr->push_inner_block_ptr(then_block_ptr);
      set_current_block_ptr(then_block_ptr);
    } else {
      // unconditional blocks cannot have inner blocks
      assert(m_current_block_ptr->inner_block_ptrs().empty());

      if (m_current_block_ptr->body().empty()) {
        // reuse current unconditional and empty block
        m_current_block_ptr->m_condition_ptr = std::move(condition_ptr);
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

    return true;
  }

  /// Optional control flow

  /// A call of this function demarcates the end of the "then" block and the
  /// beginning of the "else" block. Therefore, begin_else() must never be
  /// called prior to calling begin_then(std::unique_ptr<ReadInstr<bool>>). In
  /// addition, this member function must be called at most once. The "else"
  /// block is executed if and only if the return value is true.
  ///
  /// \return execute "else" block?
  bool begin_else() {
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
      negate(m_current_block_ptr->condition_ptr())));

    m_current_block_ptr->m_else_block_ptr = else_block_ptr;
    set_current_block_ptr(else_block_ptr);

    return true;
  }

  /// Create next nested, unconditional block inside outer block

  /// end_branch() must always be called exactly once such that its call site is
  /// the immediate post-dominator of the if-then-else statement it annotates.
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

  void end_thread() {}

  /// Insert event into current block
  void insert_event_ptr(const std::shared_ptr<Event>& event_ptr) {
    m_current_block_ptr->insert_event_ptr(event_ptr);
  }

  /// Records a direct memory write event

  /// Inserts direct write event and the read events that determine its value.
  template<typename T>
  std::shared_ptr<DirectWriteEvent<T>> instr(const MemoryAddr& addr,
    std::unique_ptr<ReadInstr<T>> instr_ptr) {

    insert_all<T>(*instr_ptr);

    std::shared_ptr<DirectWriteEvent<T>> write_event_ptr(new DirectWriteEvent<T>(
      m_thread_id, addr, std::move(instr_ptr), block_condition_ptr()));

    insert_event_ptr(write_event_ptr);
    return write_event_ptr;
  }

  /// Records an indirect memory write event

  /// Inserts indirect write event and the read events that determine its value.
  template<typename T, typename U, size_t N>
  std::shared_ptr<IndirectWriteEvent<T, U, N>> instr(const MemoryAddr& addr,
    std::unique_ptr<DerefReadInstr<T[N], U>> deref_instr_ptr,
    std::unique_ptr<ReadInstr<T>> instr_ptr) {

    insert_all<T>(*instr_ptr);
    insert_all<T>(*deref_instr_ptr);

    std::shared_ptr<IndirectWriteEvent<T, U, N>> write_event_ptr(
      new IndirectWriteEvent<T, U, N>(m_thread_id, addr,
        std::move(deref_instr_ptr), std::move(instr_ptr),
          block_condition_ptr()));

    insert_event_ptr(write_event_ptr);
    return write_event_ptr;
  }
};

}

#endif
