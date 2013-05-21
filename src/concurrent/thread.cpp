// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "concurrent/thread.h"

namespace se {

Threads Threads::s_singleton;
Z3C0 Thread::s_z3;

namespace ThisThread {
  Recorder& recorder() { return Threads::current_recorder(); }
  unsigned thread_id() { return Threads::current_thread_id(); }
};

}
