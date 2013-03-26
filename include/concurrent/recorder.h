// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_RECORDER_H_
#define LIBSE_CONCURRENT_RECORDER_H_

#include "concurrent/instr.h"
#include "concurrent/block.h"
#include "concurrent/encoder.h"

namespace se {

static void internal_relate(const std::shared_ptr<Block>& block_ptr,
  MemoryAddrRelation<Event>& relation) {

  for (const std::shared_ptr<Event>& event_ptr : block_ptr->body()) {
    relation.relate(event_ptr);
  }

  for (const std::shared_ptr<Block>& inner_block_ptr :
    block_ptr->inner_block_ptrs()) {

    internal_relate(inner_block_ptr, relation);
  }

  if (block_ptr->else_block_ptr()) {
    internal_relate(block_ptr->else_block_ptr(), relation);
  }
}

static void internal_encode(const std::shared_ptr<Block>& block_ptr,
  const Z3ValueEncoder& encoder, Z3& z3) {

  for (const std::shared_ptr<Event>& event_ptr : block_ptr->body()) {
    if (event_ptr->is_write()) {
      z3::expr equality(event_ptr->encode(encoder, z3)); 
      z3.solver.add(equality);
    }
  }

  for (const std::shared_ptr<Block>& inner_block_ptr :
    block_ptr->inner_block_ptrs()) {

    internal_encode(inner_block_ptr, encoder, z3);
  }

  if (block_ptr->else_block_ptr()) {
    internal_encode(block_ptr->else_block_ptr(), encoder, z3);
  }
}

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
    m_current_block_ptr->insert_event_ptr(event_ptr);
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
    internal_encode(most_outer_block_ptr(), encoder, z3);
  }

  void relate(MemoryAddrRelation<Event>& relation) const {
    internal_relate(most_outer_block_ptr(), relation);
  }
};

/// Static object which can record events and path conditions
extern void init_recorder();
extern std::shared_ptr<Recorder> recorder_ptr();
extern void push_recorder(unsigned thread_id);
extern void pop_recorder();

}

#endif
