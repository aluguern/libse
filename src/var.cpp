// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "var.h"

namespace se {

unsigned int var_seq = 0;

void reset_var_seq() { var_seq = 0; }

void reset_tracer() {
  tracer().reset();
  reset_var_seq();
}

std::string create_var_name() {
  sstream << SymbolicVarPrefix;
  sstream << var_seq;
  var_seq++;

  std::string name = sstream.str();

  // reset buffer
  sstream.clear();
  sstream.str("");

  return name;
}

}

