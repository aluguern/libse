// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_RECORDER_H_
#define LIBSE_CONCURRENT_RECORDER_H_

#include <memory>
#include <stack>

#include "concurrent/instr.h"

namespace se {

class PathCondition {
private:
  static const std::shared_ptr<ReadInstr<bool>> s_true_condition;

  std::stack<std::shared_ptr<ReadInstr<bool>>> m_conditions;

public:
  void push(std::unique_ptr<ReadInstr<bool>> condition) {
    m_conditions.push(std::move(condition));
  }

  void pop() { m_conditions.pop(); }

  std::shared_ptr<ReadInstr<bool>> top() const {
    if (m_conditions.empty()) {
      return s_true_condition;
    }

    return m_conditions.top();
  }
};

/// Static object which can record path constraints
extern PathCondition& path_condition();

}

#endif
