// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_ENCODER_C0_H_
#define LIBSE_CONCURRENT_ENCODER_C0_H_

#include <string>
#include <smt>

#include "concurrent/encoder.h"
#include "concurrent/relation.h"

namespace se {

/* Alex's Square */


/*
class Z3OrderEncoderC0 {
private:
  const ReadInstrEncoder m_read_encoder;

  smt::UnsafeTerm event_condition(const Event& event, Encoders& encoders) const {
    if (event.condition_ptr()) {
      return event.condition_ptr()->encode(m_read_encoder, encoders);
    }

    return smt::literal<smt::Bool>(true);
  }

  typedef std::shared_ptr<Event> EventPtr;
  typedef std::unordered_set<EventPtr> EventPtrSet;

public:
  Z3OrderEncoderC0() : m_read_encoder() {}

  /// \internal \return RF axiom encoding
  smt::UnsafeTerm rf_enc(const ZoneRelation<Event>& relation, Encoders& encoders) const {
    smt::UnsafeTerm rf_expr(smt::literal<smt::Bool>(true));
    for (const EventPtr& x_ptr : relation.event_ptrs()) {
      if (x_ptr->is_write()) { continue; }
      const Event& read_event = *x_ptr;

      assert(!read_event.zone().is_bottom());

      const smt::UnsafeTerm read_event_condition(event_condition(read_event, encoders));

      smt::UnsafeTerm wr_schedules(smt::literal<smt::Bool>(false));
      for (const EventPtr& y_ptr : relation.event_ptrs()) {
        if (y_ptr->is_read()) { continue; }
        const Event& write_event = *y_ptr;

        assert(!write_event.zone().is_bottom());
        if (read_event.zone().meet(write_event.zone()).is_bottom()) { continue; }

        const smt::UnsafeTerm wr_order(encoders.clock(write_event).simultaneous_or_happens_before(
          encoders.clock(read_event)));
        const smt::UnsafeTerm wr_sup_clock(encoders.clock(write_event).simultaneous(
          encoders.sup_clock(read_event)));
        const smt::UnsafeTerm wr_schedule(encoders.rf(write_event, read_event));
        const smt::UnsafeTerm wr_equality(write_event.constant(encoders) ==
          read_event.constant(encoders));
        const smt::UnsafeTerm write_event_condition(event_condition(write_event, encoders));

        wr_schedules = wr_schedules or wr_schedule;
        rf_expr = rf_expr and smt::implies(wr_schedule, wr_order and wr_sup_clock and
          write_event_condition and read_event_condition and wr_equality) and
          smt::implies(encoders.clock(write_event).simultaneous_or_happens_before(
              encoders.clock(read_event)) and write_event_condition,
            encoders.clock(write_event).simultaneous_or_happens_before(encoders.sup_clock(read_event)));
      }

      rf_expr = rf_expr and smt::implies(read_event_condition, wr_schedules);
    }

    encoders.transitivity(relation.event_ptrs());

    return rf_expr;
  }

  /// \internal \return WS axiom encoding
  smt::UnsafeTerm ws_enc(const ZoneRelation<Event>& relation, Encoders& encoders) const {
    const ZoneAtomSet& zone_atoms = relation.zone_atoms();

    smt::UnsafeTerm ws_expr(smt::literal<smt::Bool>(true));
    for (const Zone& zone : zone_atoms) {
      const EventPtrSet write_event_ptrs = relation.find(zone,
        WriteEventPredicate::predicate());

      smt::UnsafeTerms ptrs;
      ptrs.reserve(write_event_ptrs.size());

      for (const EventPtr& write_event_ptr : write_event_ptrs) {
        const Event& write_event = *write_event_ptr;
        ptrs.push_back(encoders.clock(write_event).term());
      }

      if (1 < ptrs.size()) {
        const smt::UnsafeTerm zone_ws_expr(smt::distinct(std::move(ptrs)));
        ws_expr = ws_expr and zone_ws_expr;
      }
    }

    return ws_expr;
  }

  void encode_without_ws(const ZoneRelation<Event>& zone_relation, Encoders& encoders) const
  {
    encoders.solver.unsafe_add(rf_enc(zone_relation, encoders));
  }

  void encode(const ZoneRelation<Event>& zone_relation, Encoders& encoders) const
  {
    encode_without_ws(zone_relation, encoders);
    encoders.solver.unsafe_add(ws_enc(zone_relation, encoders));
  }

};
*/
















/* Alex's Cube */

#define IMPLICIT_WS 1
class Z3OrderEncoderC0 {
private:
  const ReadInstrEncoder m_read_encoder;

  smt::UnsafeTerm event_condition(const Event& event, Encoders& encoders) const {
    if (event.condition_ptr()) {
      return event.condition_ptr()->encode(m_read_encoder, encoders);
    }

    return smt::literal<smt::Bool>(true);
  }

  typedef std::shared_ptr<Event> EventPtr;
  typedef std::unordered_set<EventPtr> EventPtrSet;

public:
  Z3OrderEncoderC0() : m_read_encoder() {}

  /// \internal \return RF axiom encoding
  smt::UnsafeTerm rf_enc(const ZoneRelation<Event>& relation, Encoders& encoders) const {
    smt::UnsafeTerm rf_expr(smt::literal<smt::Bool>(true));
    for (const EventPtr& x_ptr : relation.event_ptrs()) {
      if (x_ptr->is_write()) { continue; }
      const Event& read_event = *x_ptr;

      assert(!read_event.zone().is_bottom());
      const smt::UnsafeTerm read_event_condition(event_condition(read_event, encoders));

      smt::UnsafeTerm wr_schedules(smt::literal<smt::Bool>(false));
      for (const EventPtr& y_ptr : relation.event_ptrs()) {
        if (y_ptr->is_read()) { continue; }
        const Event& write_event = *y_ptr;

        assert(!write_event.zone().is_bottom());
        if (read_event.zone().meet(write_event.zone()).is_bottom()) { continue; }

        const smt::UnsafeTerm wr_order(
          encoders.clock(write_event).happens_before(encoders.clock(read_event)));

        const smt::UnsafeTerm wr_schedule(encoders.rf(write_event, read_event));
        const smt::UnsafeTerm wr_equality(write_event.constant(encoders) ==
          read_event.constant(encoders));
        const smt::UnsafeTerm write_event_condition(event_condition(write_event, encoders));

        wr_schedules = wr_schedules or wr_schedule;
        rf_expr = rf_expr and smt::implies(wr_schedule, wr_order and
          write_event_condition and wr_equality);
      }

      rf_expr = rf_expr and smt::implies(read_event_condition, wr_schedules);
    }
    return rf_expr;
  }

  /// \internal \return FR axiom encoding
  smt::UnsafeTerm fr_enc(const ZoneRelation<Event>& relation, Encoders& encoders) const {
    const ZoneAtomSet& zone_atoms = relation.zone_atoms();

    smt::UnsafeTerm fr_expr(smt::literal<smt::Bool>(true));
    for (const Zone& zone : zone_atoms) {
      const std::pair<EventPtrSet, EventPtrSet> result =
        relation.partition(zone);
      const EventPtrSet& read_event_ptrs = result.first;
      const EventPtrSet& write_event_ptrs = result.second;

      for (const EventPtr& write_event_ptr_x : write_event_ptrs) {
        for (const EventPtr& write_event_ptr_y : write_event_ptrs) {
          if (write_event_ptr_x == write_event_ptr_y) { continue; }

          const Event& write_event_x = *write_event_ptr_x;
          const Event& write_event_y = *write_event_ptr_y;

          assert(!write_event_x.zone().is_bottom());
          assert(!write_event_y.zone().is_bottom());

          for (const EventPtr& read_event_ptr : read_event_ptrs) {
            const Event& read_event = *read_event_ptr;

            assert(!read_event.zone().is_bottom());

            const smt::UnsafeTerm xr_schedule(encoders.rf(write_event_x, read_event));
            const smt::UnsafeTerm yx_order(encoders.clock(write_event_y).happens_before(encoders.clock(write_event_x)));
            const smt::UnsafeTerm yr_order(encoders.clock(write_event_y).simultaneous_or_happens_before(encoders.clock(read_event)));
            const smt::UnsafeTerm y_condition(event_condition(write_event_y, encoders));

            fr_expr = fr_expr and
              smt::implies(xr_schedule and yr_order and y_condition, yx_order);
          }
        }
      }
    }

    return fr_expr;
  }

  void encode_without_ws(const ZoneRelation<Event>& zone_relation, Encoders& encoders) const
  {
    encoders.solver.unsafe_add(rf_enc(zone_relation, encoders));
    encoders.solver.unsafe_add(fr_enc(zone_relation, encoders));
  }

  void encode(const ZoneRelation<Event>& zone_relation, Encoders& encoders) const
  {
    encode_without_ws(zone_relation, encoders);
  }
};



















/* Michael's Cube */

/*
class Z3OrderEncoderC0 {
private:
  const ReadInstrEncoder m_read_encoder;

  smt::UnsafeTerm event_condition(const Event& event, Encoders& encoders) const {
    if (event.condition_ptr()) {
      return event.condition_ptr()->encode(m_read_encoder, encoders);
    }

    return smt::literal<smt::Bool>(true);
  }

  typedef std::shared_ptr<Event> EventPtr;
  typedef std::unordered_set<EventPtr> EventPtrSet;

public:
  Z3OrderEncoderC0() : m_read_encoder() {}

  /// \internal \return RF axiom encoding
  smt::UnsafeTerm rf_enc(const ZoneRelation<Event>& relation, Encoders& encoders) const {
    smt::UnsafeTerm rf_expr(smt::literal<smt::Bool>(true));
    for (const EventPtr& x_ptr : relation.event_ptrs()) {
      if (x_ptr->is_write()) { continue; }
      const Event& read_event = *x_ptr;

      assert(!read_event.zone().is_bottom());
      const smt::UnsafeTerm read_event_condition(event_condition(read_event, encoders));

      smt::UnsafeTerm wr_schedules(smt::literal<smt::Bool>(false));
      for (const EventPtr& y_ptr : relation.event_ptrs()) {
        if (y_ptr->is_read()) { continue; }
        const Event& write_event = *y_ptr;

        assert(!write_event.zone().is_bottom());
        if (read_event.zone().meet(write_event.zone()).is_bottom()) { continue; }

        const smt::UnsafeTerm wr_order(
          encoders.clock(write_event).happens_before(encoders.clock(read_event)));

        const smt::UnsafeTerm wr_schedule(encoders.rf(write_event, read_event));
        const smt::UnsafeTerm wr_equality(write_event.constant(encoders) ==
          read_event.constant(encoders));
        const smt::UnsafeTerm write_event_condition(event_condition(write_event, encoders));

        wr_schedules = wr_schedules or wr_schedule;
        rf_expr = rf_expr and smt::implies(wr_schedule, wr_order and
          write_event_condition and read_event_condition and wr_equality);
      }

      rf_expr = rf_expr and smt::implies(read_event_condition, wr_schedules);
    }
    return rf_expr;
  }

  /// \internal \return WS axiom encoding
  smt::UnsafeTerm ws_enc(const ZoneRelation<Event>& relation, Encoders& encoders) const {
    const ZoneAtomSet& zone_atoms = relation.zone_atoms();

    smt::UnsafeTerm ws_expr(smt::literal<smt::Bool>(true));
    for (const Zone& zone : zone_atoms) {
      const EventPtrSet write_event_ptrs = relation.find(zone,
        WriteEventPredicate::predicate());

      smt::UnsafeTerms ptrs;
      ptrs.reserve(write_event_ptrs.size());

      for (const EventPtr& write_event_ptr : write_event_ptrs) {
        const Event& write_event = *write_event_ptr;
        ptrs.push_back(encoders.clock(write_event).term());
      }

      if (1 < ptrs.size()) {
        const smt::UnsafeTerm zone_ws_expr(smt::distinct(std::move(ptrs)));
        ws_expr = ws_expr and zone_ws_expr;
      }
    }

    return ws_expr;
  }

  /// \internal \return FR axiom encoding
  smt::UnsafeTerm fr_enc(const ZoneRelation<Event>& relation, Encoders& encoders) const {
    const ZoneAtomSet& zone_atoms = relation.zone_atoms();

    smt::UnsafeTerm fr_expr(smt::literal<smt::Bool>(true));
    for (const Zone& zone : zone_atoms) {
      const std::pair<EventPtrSet, EventPtrSet> result =
        relation.partition(zone);
      const EventPtrSet& read_event_ptrs = result.first;
      const EventPtrSet& write_event_ptrs = result.second;

      for (const EventPtr& write_event_ptr_x : write_event_ptrs) {
        for (const EventPtr& write_event_ptr_y : write_event_ptrs) {
          if (write_event_ptr_x == write_event_ptr_y) { continue; }

          const Event& write_event_x = *write_event_ptr_x;
          const Event& write_event_y = *write_event_ptr_y;

          assert(!write_event_x.zone().is_bottom());
          assert(!write_event_y.zone().is_bottom());

          for (const EventPtr& read_event_ptr : read_event_ptrs) {
            const Event& read_event = *read_event_ptr;

            assert(!read_event.zone().is_bottom());

            const smt::UnsafeTerm xr_schedule(encoders.rf(write_event_x, read_event));
            const smt::UnsafeTerm xy_order(encoders.clock(write_event_x).happens_before(encoders.clock(write_event_y)));
            const smt::UnsafeTerm ry_order(encoders.clock(read_event).happens_before(encoders.clock(write_event_y)));
            const smt::UnsafeTerm y_condition(event_condition(write_event_y, encoders));

            fr_expr = fr_expr and
              smt::implies(xr_schedule and xy_order and y_condition, ry_order);
          }
        }
      }
    }

    return fr_expr;
  }

  void encode_without_ws(const ZoneRelation<Event>& zone_relation, Encoders& encoders) const
  {
    encoders.solver.unsafe_add(rf_enc(zone_relation, encoders));
    encoders.solver.unsafe_add(fr_enc(zone_relation, encoders));
  }

  void encode(const ZoneRelation<Event>& zone_relation, Encoders& encoders) const
  {
    encode_without_ws(zone_relation, encoders);
    encoders.solver.unsafe_add(ws_enc(zone_relation, encoders));
  }
};
*/

}

#endif
