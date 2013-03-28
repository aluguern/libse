// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_BLOCK_H_
#define LIBSE_CONCURRENT_BLOCK_H_

#include <forward_list>
#include <list>

#include "concurrent/event.h"
#include "concurrent/instr.h"

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
  std::forward_list<std::shared_ptr<Event>>::const_iterator m_body_cend;

  std::list<std::shared_ptr<Block>> m_inner_block_ptrs;

  std::shared_ptr<Block> m_else_block_ptr;

  Block(std::shared_ptr<Block> outer_block_ptr,
    std::unique_ptr<ReadInstr<bool>> condition_ptr = nullptr) :
    m_outer_block_ptr(outer_block_ptr),
    m_condition_ptr(std::move(condition_ptr)),
    m_body(/* empty */), m_body_cend(m_body.cbefore_begin()),
    m_inner_block_ptrs(/* empty */), m_else_block_ptr(/* none */) {}

  void push_inner_block_ptr(const std::shared_ptr<Block>& block_ptr) {
    m_inner_block_ptrs.push_back(block_ptr);
  }

  void pop_inner_block_ptr() {
    m_inner_block_ptrs.pop_back();
  }

public:
  /// Root of a new series-parallel graph
  static std::unique_ptr<Block> make_root() {
    return std::unique_ptr<Block>(new Block(nullptr, nullptr));
  }

  /// Append all the given event pointers to the body
  void insert_all(const std::forward_list<std::shared_ptr<Event>>& event_ptrs) {
    m_body_cend = m_body.insert_after(m_body_cend,
      /* range */ event_ptrs.cbegin(), event_ptrs.cend());
  }

  /// Insert event into at the end of the list
  void insert_event_ptr(const std::shared_ptr<Event>& event_ptr) {
    m_body_cend = m_body.insert_after(m_body_cend, event_ptr);
  }

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

}

#endif
