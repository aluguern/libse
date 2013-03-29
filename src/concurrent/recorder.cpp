// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stack>

#include "concurrent/recorder.h"

namespace se {

static std::stack<std::shared_ptr<Recorder>> s_recorder_ptrs;

//std::shared_ptr<Recorder> recorder_ptr() { return s_recorder_ptrs.top(); }

}
