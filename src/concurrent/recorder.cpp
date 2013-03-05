// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "concurrent/recorder.h"

namespace se {

const std::shared_ptr<ReadInstr<bool>> PathCondition::s_true_condition = nullptr;

PathCondition& path_condition() {
  static PathCondition s_path_condition;
  return s_path_condition;
}

}
