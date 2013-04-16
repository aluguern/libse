// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "concurrent/zone.h"

namespace se {

Zone Zone::s_bottom_element;
unsigned Zone::s_next_atom = 0;

}
