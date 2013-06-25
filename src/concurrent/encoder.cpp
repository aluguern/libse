// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "concurrent/encoder_c0.h"

namespace se {

smt::UnsafeTerm SyncEvent::VALUE_ENCODER_FN_DEF
smt::UnsafeTerm SyncEvent::CONSTANT_ENCODER_FN_DEF

#ifdef __USE_MATRIX__
const std::string Clock::s_happens_before_prefix = "happens_before_";
const std::string Clock::s_simultaneous_prefix = "simultaneous_";
#endif

}
