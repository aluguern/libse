// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_RECORDER_H_
#define LIBSE_CONCURRENT_RECORDER_H_

#include <forward_list>
#include <list>

#include "concurrent/event.h"
#include "concurrent/instr.h"
#include "concurrent/encoder.h"

namespace se {

/// Logical control flow unit in structured programs

/// A block is a vertex in a series-parallel graph. A block is defined to have a
/// list of \ref Event "events", an \ref Block::outer_block_ptr() "outer block",
/// none or more \ref Block::inner_block_ptrs() "inner blocks", and an optional
/// \ref Block::else_block_ptr() "else block".
///
/// The list of events is called the \ref Block::body() "body" of the block.
/// A block is said to be "empty" if its body is empty; otherwise, the block is
/// called "nonempty". The events in the body are interpreted to occur before
/// those in any inner blocks. Also, if I and I' are inner blocks of block B,
/// and I is before I' in the list of inner blocks of B, then the events in I
/// occur before those in I'.
///
/// Each block has a necessary \ref Block::condition_ptr() "condition" for the
/// block to execute. The block is said to be "conditional" if its condition is
/// not null; otherwise, it is called "unconditional". The inner blocks can be
/// conditional, unconditional, or any mixture thereof. If B has an else block
/// C, then C has as condition the negation of B's condition.
class Block {
private:
  friend class Recorder;

  const std::shared_ptr<Block> m_outer_block_ptr;
  std::shared_ptr<ReadInstr<bool>> m_condition_ptr;

  std::forward_list<std::shared_ptr<Event>> m_body;
  std::list<std::shared_ptr<Block>> m_inner_block_ptrs;

  std::shared_ptr<Block> m_else_block_ptr;

  Block(std::shared_ptr<Block> outer_block_ptr,
    std::unique_ptr<ReadInstr<bool>> condition_ptr = nullptr) :
    m_outer_block_ptr(outer_block_ptr),
    m_condition_ptr(std::move(condition_ptr)),
    m_body(/* empty */), m_inner_block_ptrs(/* empty */),
    m_else_block_ptr(/* none */) {}

  /// Insert in the body all those read events that are in the given instruction
  template<typename T>
  void bulk_insert(const ReadInstr<T>& instr) {
    // extract from ReadInstr<T> all pointers to ReadEvent<T> objects
    std::forward_list<std::shared_ptr<Event>> read_event_ptrs;
    instr.filter(read_event_ptrs);

    // append these pointers to the block's event pointer list
    m_body.insert_after(m_body.cbefore_begin(),
      /* range */ read_event_ptrs.cbegin(), read_event_ptrs.cend());
  }

  void push_inner_block_ptr(const std::shared_ptr<Block>& block_ptr) {
    m_inner_block_ptrs.push_back(block_ptr);
  }

  void pop_inner_block_ptr() {
    m_inner_block_ptrs.pop_back();
  }

public:
  /// null if and only if this is the most outer block

  /// If this is the most outer block, then body() is empty but 
  /// inner_block_ptrs() has at least one block that can be
  /// conditional or even unconditional.
  std::shared_ptr<Block> outer_block_ptr() const {
    return m_outer_block_ptr;
  }

  // If condition_ptr() is null, then inner_block_ptrs() is empty
  std::shared_ptr<ReadInstr<bool>> condition_ptr() const {
    return m_condition_ptr;
  }

  const std::forward_list<std::shared_ptr<Event>>& body() const {
    return m_body;
  }

  const std::list<std::shared_ptr<Block>>& inner_block_ptrs() const {
    return m_inner_block_ptrs;
  }

  std::shared_ptr<Block> else_block_ptr() const { return m_else_block_ptr; }
};

/// Records events and path conditions on a per-thread basis

/// The output of a recorder is a series-parallel graph of \ref Block "blocks".
class Recorder {
private:
  static const std::shared_ptr<ReadInstr<bool>> s_true_condition_ptr;

  const unsigned m_thread_id;
  const std::shared_ptr<Block> m_most_outer_block_ptr;

  std::shared_ptr<Block> m_current_block_ptr;

  static std::unique_ptr<ReadInstr<bool>> negate(
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr) {

    return std::unique_ptr<ReadInstr<bool>>(new UnaryReadInstr<NOT, bool>(
      condition_ptr));
  }

public:
  Recorder(unsigned thread_id) : m_thread_id(thread_id),
    m_most_outer_block_ptr(new Block(nullptr, nullptr)),
    m_current_block_ptr(std::unique_ptr<Block>(new Block(
      m_most_outer_block_ptr))) {

    m_most_outer_block_ptr->push_inner_block_ptr(m_current_block_ptr);
  }

  unsigned thread_id() const { return m_thread_id; }

  std::shared_ptr<Block> most_outer_block_ptr() const {
    return m_most_outer_block_ptr;
  }

  /// Conjunction of nested block conditions
  std::shared_ptr<ReadInstr<bool>> block_condition_ptr() const {
    // TODO: Keep on conjoining these conditions until reaching most outer block
    return m_current_block_ptr->condition_ptr();
  }

  const Block& current_block_ref() const { return *m_current_block_ptr; }
  std::shared_ptr<Block> current_block_ptr() const { return m_current_block_ptr; }

  /// \internal
  std::forward_list<std::shared_ptr<Event>>& current_block_body() const {
    return m_current_block_ptr->m_body;
  }

  bool begin_then(std::unique_ptr<ReadInstr<bool>> condition_ptr) {
    if (m_current_block_ptr->condition_ptr()) {
      // start nested branch inside current conditional block
      const std::shared_ptr<Block> then_block_ptr(new Block(
        m_current_block_ptr, std::move(condition_ptr)));

      m_current_block_ptr->push_inner_block_ptr(then_block_ptr);
      m_current_block_ptr = then_block_ptr;
    } else {
      // unconditional blocks cannot have inner blocks
      assert(m_current_block_ptr->inner_block_ptrs().empty());

      if (m_current_block_ptr->body().empty()) {
        // reuse current unconditional and empty block
        m_current_block_ptr->m_condition_ptr = std::move(condition_ptr);
      } else {
        std::shared_ptr<Block> outer_block_ptr(
          m_current_block_ptr->outer_block_ptr());

        // next branch after an unconditional block inside outer block
        std::shared_ptr<Block> then_block_ptr(new Block(outer_block_ptr,
          std::move(condition_ptr)));

        outer_block_ptr->push_inner_block_ptr(then_block_ptr);
        m_current_block_ptr = then_block_ptr;       
      }
    }

    return true;
  }

  bool begin_else() {
    if (!m_current_block_ptr->condition_ptr()) {
      // unconditional blocks cannot have inner blocks
      assert(m_current_block_ptr->inner_block_ptrs().empty());

      if (m_current_block_ptr->body().empty()) {
        m_current_block_ptr = m_current_block_ptr->outer_block_ptr();

        // delete last empty inner block inside "then" branch
        m_current_block_ptr->pop_inner_block_ptr();
      } else {
        m_current_block_ptr = m_current_block_ptr->outer_block_ptr();
      }
    }

    // we're now in the block for the "then" branch
    assert(nullptr != m_current_block_ptr->outer_block_ptr());
    std::shared_ptr<Block> else_block_ptr(new Block(
      m_current_block_ptr->outer_block_ptr(),
      negate(m_current_block_ptr->condition_ptr())));

    m_current_block_ptr->m_else_block_ptr = else_block_ptr;
    m_current_block_ptr = else_block_ptr;

    return true;
  }

  // Create next nested, unconditional block inside outer block
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
    }

    std::shared_ptr<Block> unconditional_block_ptr(new Block(outer_block_ptr));
    outer_block_ptr->push_inner_block_ptr(unconditional_block_ptr);
    m_current_block_ptr = unconditional_block_ptr;
  }

  void end_thread() {}

  /// Insert event into current block
  void insert_event_ptr(const std::shared_ptr<Event>& event_ptr) {
    m_current_block_ptr->m_body.push_front(event_ptr);
  }

  /// Records a direct memory write event
  template<typename T>
  std::shared_ptr<DirectWriteEvent<T>> instr(const MemoryAddr& addr,
    std::unique_ptr<ReadInstr<T>> instr_ptr) {

    m_current_block_ptr->bulk_insert<T>(*instr_ptr);

    std::shared_ptr<DirectWriteEvent<T>> write_event_ptr(new DirectWriteEvent<T>(
      m_thread_id, addr, std::move(instr_ptr), block_condition_ptr()));

    insert_event_ptr(write_event_ptr);
    return write_event_ptr;
  }

  /// Records an indirect memory write event
  template<typename T, typename U, size_t N>
  std::shared_ptr<IndirectWriteEvent<T, U, N>> instr(const MemoryAddr& addr,
    std::unique_ptr<DerefReadInstr<T[N], U>> deref_instr_ptr,
    std::unique_ptr<ReadInstr<T>> instr_ptr) {

    m_current_block_ptr->bulk_insert<T>(*instr_ptr);

    std::shared_ptr<IndirectWriteEvent<T, U, N>> write_event_ptr(
      new IndirectWriteEvent<T, U, N>(m_thread_id, addr,
        std::move(deref_instr_ptr), std::move(instr_ptr),
          block_condition_ptr()));

    insert_event_ptr(write_event_ptr);
    return write_event_ptr;
  }

  void encode(const Z3ValueEncoder& encoder, Z3& z3) const {
    for (std::shared_ptr<Event> event_ptr : current_block_body()) {
      if (event_ptr->is_write()) {
        z3::expr equality(event_ptr->encode(encoder, z3)); 
        z3.solver.add(equality);
      }
    }
  }

  void relate(MemoryAddrRelation<Event>& relation) const {
    for (std::shared_ptr<Event> event_ptr : current_block_body()) {
      relation.relate(event_ptr);
    }
  }
};

/// Static object which can record events and path conditions
extern void init_recorder();
extern std::shared_ptr<Recorder> recorder_ptr();
extern void push_recorder(unsigned thread_id);
extern void pop_recorder();

}

#endif
