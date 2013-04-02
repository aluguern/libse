// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_H_
#define LIBSE_H_

#include "concurrent.h"

namespace se {

class __Start {
public:
  __Start() {
    Threads::reset();
    Threads::begin_main_thread();
  }

  /* NB: destructor would not be called until after exiting main(void) */
};

/// \internal Constructor of __Start calls Threads::begin_main_thread()
extern __Start main_thread;

}

#endif
