// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "concurrent/relation.h"

namespace se {

ReadEventPredicate ReadEventPredicate::s_read_predicate;
WriteEventPredicate WriteEventPredicate::s_write_predicate;

}
