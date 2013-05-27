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
typedef std::list<EventPtr> EventPtrs;

class Slice {
public:
  typedef std::unordered_map<ThreadId, EventPtrs> EventPtrsMap;

private:
  EventPtrsMap m_event_ptrs_map;
  EventPtrsMap m_save_event_ptrs_map;

public:
  Slice() :
    m_event_ptrs_map(),
    m_save_event_ptrs_map() {}

  /// Resets the slice to the initialization events
  /// \see_also save_slice()
  void reset() {
    m_event_ptrs_map = m_save_event_ptrs_map;
  }

  /// Save the current events so that they can be restored with reset()
  void save_slice() {
    m_save_event_ptrs_map = m_event_ptrs_map;
  }

  void append(ThreadId thread_id, const EventPtr& event_ptr) {
    m_event_ptrs_map[thread_id].push_back(event_ptr);
  }

  /// Append all read events that are in the given instruction
  template<typename T>
  void append_all(ThreadId thread_id, const ReadInstr<T>& instr) {
    std::forward_list<std::shared_ptr<Event>> read_event_ptrs;
    instr.filter(read_event_ptrs);
    append_all(thread_id, read_event_ptrs);
  }

  /// Append all the given event pointers
  void append_all(ThreadId thread_id,
    const std::forward_list<std::shared_ptr<Event>>& event_ptrs) {

    EventPtrs& thread_event_ptrs = m_event_ptrs_map[thread_id];
    thread_event_ptrs.insert(thread_event_ptrs.end(),
      /* range */ event_ptrs.begin(), event_ptrs.end());
  }

  const EventPtrsMap& event_ptrs_map() const {
    return m_event_ptrs_map;
  }
};

}

#endif
