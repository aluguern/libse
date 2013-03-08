// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "concurrent/recorder.h"

namespace se {

const std::shared_ptr<ReadInstr<bool>> PathCondition::s_true_condition = nullptr;

static std::shared_ptr<Recorder> s_recorder_ptr(new Recorder(/* main thread */ 0));

std::shared_ptr<Recorder> recorder_ptr() { return s_recorder_ptr; }
void set_recorder(const std::shared_ptr<Recorder>& recorder_ptr) {
  s_recorder_ptr = recorder_ptr;
}

}
