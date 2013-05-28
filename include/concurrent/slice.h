// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_TRACE_H_
#define LIBSE_CONCURRENT_TRACE_H_

#include <list>
#include <forward_list>
#include <unordered_map>

#include "concurrent/event.h"

namespace se {

typedef std::shared_ptr<Event> EventPtr;

/// Path in a series-parallel graph
class Slice {
public:
  typedef std::list<EventPtr> EventPtrs;

private:
  EventPtrs m_event_ptrs;
  EventPtrs m_save_event_ptrs;

public:
  Slice() :
    m_event_ptrs(),
    m_save_event_ptrs() {}

  /// Resets the slice to the initialization events
  ///
  /// \see_also save()
  void reset() {
    m_event_ptrs = m_save_event_ptrs;
  }

  /// Save the current events so that they can be restored with reset()
  void save() {
    m_save_event_ptrs = m_event_ptrs;
  }

  void append(const EventPtr& event_ptr) {
    m_event_ptrs.push_back(event_ptr);
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
    m_event_ptrs.insert(m_event_ptrs.end(),
      /* range */ event_ptrs.begin(), event_ptrs.end());
  }

  const EventPtrs& event_ptrs() const {
    return m_event_ptrs;
  }
};

}

#endif
