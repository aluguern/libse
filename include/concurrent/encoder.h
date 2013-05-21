// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_ENCODER_H_
#define LIBSE_CONCURRENT_ENCODER_H_

#include "core/op.h"

namespace se {

class Event;

class Z3 {
public:
  z3::context context;
  z3::solver solver;

  Z3() : context(), solver(context) {}

  /// Creates a Z3 constant according to the event's \ref Event::type() "type"
  virtual z3::expr constant(const Event& event) = 0;

  /// Equality between write event and read event applied to function `rf`

  /// \returns `w == rf(r)`, i.e. `r` reads from `w`
  virtual z3::expr rf(const Event& write_event, const Event& read_event) = 0;

  /// Unique clock constraint for an event
  virtual z3::expr clock(const Event& event) = 0;

  /// Least upper bound of two elements in a well-ordered set
  virtual z3::expr join_clocks(const z3::expr& x, const z3::expr& y) = 0;
};

template<Opcode opcode, typename T>
struct Z3Identity {
  static z3::expr constant(z3::context&);
};

template<>
struct Z3Identity<LAND, bool> {
  static z3::expr constant(z3::context& context) {
    return context.bool_val(true);
  }
};

}

#endif
