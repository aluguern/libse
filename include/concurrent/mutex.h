// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_MUTEX_H_
#define LIBSE_MUTEX_H_

#include "concurrent.h"

namespace se {

/// Symbolic spinlock

/// The Mutex class symbolically encodes a spinlock that protects shared data
/// from being simultaneously accessed by multiple threads.
class Mutex {
private:
  size_t m_lock_thread_id;
  SharedVar<size_t> m_thread_id;

protected:
  void unlock(Z3& z3) {
    assert(m_lock_thread_id == ThisThread::thread_id());
    Threads::error(m_thread_id == m_lock_thread_id, z3);
  }

public:
  Mutex() : m_lock_thread_id(0), m_thread_id() {}

  /// Acquire lock

  // TODO: Ensure recorder preserves PO between lock() and unlock()
  void lock() {
    m_lock_thread_id = ThisThread::thread_id();
    m_thread_id = m_lock_thread_id;
  }

  /// Release lock

  /// \pre: ThisThread is the same as the one that called lock()
  void unlock() { unlock(Thread::z3()); }
};

}

#endif
