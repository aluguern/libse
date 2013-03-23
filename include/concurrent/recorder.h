// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_RECORDER_H_
#define LIBSE_CONCURRENT_RECORDER_H_

#include <forward_list>

#include "concurrent/event.h"
#include "concurrent/instr.h"
#include "concurrent/encoder.h"

namespace se {

/// Vertex in a doubly-linked binary tree structure
class Block {
private:
  friend class Recorder;

  std::shared_ptr<Block> m_prev_block_ptr;
  const std::shared_ptr<Block> m_next_block_ptr;

  const std::shared_ptr<ReadInstr<bool>> m_condition_ptr;
  std::forward_list<std::shared_ptr<Event>> m_body;

  std::shared_ptr<Block> m_then_block_ptr;
  std::shared_ptr<Block> m_else_block_ptr;

  Block(std::shared_ptr<Block> prev_block_ptr,
    std::shared_ptr<Block> next_block_ptr,
    std::shared_ptr<ReadInstr<bool>> condition_ptr) :
    m_prev_block_ptr(prev_block_ptr), m_next_block_ptr(next_block_ptr),
    m_condition_ptr(condition_ptr), m_body(),
    m_then_block_ptr(), m_else_block_ptr() {}

  /// Extract and then insert all read events in the given instruction
  template<typename T>
  void bulk_insert(const ReadInstr<T>& instr) {
    // Extract from ReadInstr<T> all pointers to ReadEvent<T> objects
    std::forward_list<std::shared_ptr<Event>> read_event_ptrs;
    instr.filter(read_event_ptrs);

    // Append these pointers to the block's event pointer list
    m_body.insert_after(m_body.cbefore_begin(),
      /* range */ read_event_ptrs.cbegin(), read_event_ptrs.cend());
  }

public:
  /// Null if and only if block is root in doubly-linked tree
  std::shared_ptr<Block> prev_block_ptr() const { return m_prev_block_ptr; }

  /// If next_block_ptr() is null, then then_block_ptr() is null
  std::shared_ptr<Block> next_block_ptr() const { return m_next_block_ptr; }

  /// If prev_block_ptr() is null, then condition_ptr() is null
  std::shared_ptr<ReadInstr<bool>> condition_ptr() const { return m_condition_ptr; }
  const std::forward_list<std::shared_ptr<Event>>& body() const { return m_body; }

  /// If then_block_ptr() is null, then else_block_ptr() is null

  /// \see_also next_block_ptr()
  std::shared_ptr<Block> then_block_ptr() const { return m_then_block_ptr; }

  /// \see_also then_block_ptr()
  std::shared_ptr<Block> else_block_ptr() const { return m_else_block_ptr; }
};

/// Records events and path conditions on a per-thread basis

/// The output of a recorder is a series-parallel graph of \ref Block "blocks".
class Recorder {
private:
  const unsigned m_thread_id;

  std::shared_ptr<Block> m_current_block_ptr;

public:
  Recorder(unsigned thread_id) : m_thread_id(thread_id),
    m_current_block_ptr(new Block(nullptr, nullptr, nullptr)) {}

  unsigned thread_id() const { return m_thread_id; }

  /// Conjunctions of block conditions along path from current block to the root
  std::shared_ptr<ReadInstr<bool>> path_condition_ptr() const {
    return m_current_block_ptr->condition_ptr();
  }

  const Block& current_block_ref() const { return *m_current_block_ptr; }
  std::shared_ptr<Block> current_block_ptr() const { return m_current_block_ptr; }

  /// \internal
  std::forward_list<std::shared_ptr<Event>>& current_block_body() const {
    return m_current_block_ptr->m_body;
  }

  bool begin_then_block(std::unique_ptr<ReadInstr<bool>> condition_ptr) {
    // vertex to join "then" (and possibly "else") block
    std::shared_ptr<Block> next_block_ptr(new Block(nullptr, nullptr,
      m_current_block_ptr->condition_ptr()));

    m_current_block_ptr->m_then_block_ptr = std::unique_ptr<Block>(new Block(
      m_current_block_ptr, next_block_ptr, std::move(condition_ptr)));

    m_current_block_ptr = m_current_block_ptr->then_block_ptr();
    next_block_ptr->m_prev_block_ptr = m_current_block_ptr;

    return true;
  }

  bool begin_else_block() {
    std::shared_ptr<ReadInstr<bool>> else_condition_ptr(
      new UnaryReadInstr<NOT, bool>(m_current_block_ptr->condition_ptr()));

    std::shared_ptr<Block> prev_block_ptr(m_current_block_ptr->prev_block_ptr());

    prev_block_ptr->m_else_block_ptr = std::unique_ptr<Block>(new Block(
      prev_block_ptr, m_current_block_ptr->next_block_ptr(), else_condition_ptr));

    m_current_block_ptr = prev_block_ptr->else_block_ptr();

    return true;
  }

  void end_block() {
    m_current_block_ptr = m_current_block_ptr->next_block_ptr();
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
        m_thread_id, addr, std::move(instr_ptr), path_condition_ptr()));

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
          path_condition_ptr()));

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
