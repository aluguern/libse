#include "gtest/gtest.h"

#include "concurrent/relation.h"
#include "concurrent/zone.h"
#include "concurrent/instr.h"
#include "concurrent/encoder_c0.h"

using namespace se;

/// A read event on an integer
class TestEvent : public Event {
public:
  TestEvent(const Zone& zone,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(0, zone, true, &TypeInfo<int>::s_type, condition_ptr) {}

  smt::UnsafeTerm encode_eq(const ValueEncoder& encoder, Encoders& helper) const {
    return helper.constant(*this);
  }

  smt::UnsafeTerm constant(Encoders& helper) const { return helper.constant(*this); }
};

TEST(RelationTest, CopyZoneAtom) {
  const Zone zone = Zone::unique_atom();
  const ZoneAtom zone_atom = *ZoneAtomSets::zone_atom_set(zone).cbegin();
  const ZoneAtom copy(zone_atom);

  EXPECT_EQ(static_cast<unsigned>(zone_atom), static_cast<unsigned>(copy));
  EXPECT_EQ(zone_atom, copy);
}

TEST(RelationTest, ReadEventPredicate) {
  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();

  const Predicate<std::shared_ptr<Event>>& predicate(ReadEventPredicate::predicate());

  const std::shared_ptr<Event> read_event_ptr(new ReadEvent<int>(thread_id, zone));
  EXPECT_TRUE(predicate.check(read_event_ptr));

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new DirectWriteEvent<long>(thread_id, zone, std::move(read_instr_ptr)));
  EXPECT_FALSE(predicate.check(write_event_ptr));
}

TEST(RelationTest, WriteEventPredicate) {
  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();

  const Predicate<std::shared_ptr<Event>>& predicate(WriteEventPredicate::predicate());

  const std::shared_ptr<Event> read_event_ptr(new ReadEvent<int>(thread_id, zone));
  EXPECT_FALSE(predicate.check(read_event_ptr));

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new DirectWriteEvent<long>(thread_id, zone, std::move(read_instr_ptr)));
  EXPECT_TRUE(predicate.check(write_event_ptr));
}

TEST(RelationTest, DefaultZoneRelation) {
  const Zone zone = Zone::unique_atom();
  const Zone other_zone = Zone::unique_atom();

  const std::shared_ptr<Event> event_ptr(new TestEvent(zone));

  ZoneRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.event_ptrs().size());
  EXPECT_EQ(1, relation.find(zone, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(0, relation.find(other_zone, ReadEventPredicate::predicate()).size());

  relation.clear();
  EXPECT_TRUE(relation.find(zone, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.event_ptrs().empty());
}

TEST(RelationTest, ZoneRelationAsFunction) {
  const Zone zone = Zone::unique_atom();
  const Zone other_zone = Zone::unique_atom();

  const std::shared_ptr<Event> event_ptr(new TestEvent(zone));

  ZoneRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.event_ptrs().size());
  EXPECT_EQ(1, relation.find(zone, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(0, relation.find(other_zone, ReadEventPredicate::predicate()).size());

  relation.clear();
  EXPECT_TRUE(relation.find(zone, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.event_ptrs().empty());
}

TEST(RelationTest, ZoneRelation) {
  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = Zone::unique_atom();
  const Zone zone_c = Zone::unique_atom();
  const Zone zone = zone_a.join(zone_b);

  const std::shared_ptr<Event> event_ptr(new TestEvent(zone));

  ZoneRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.event_ptrs().size());
  EXPECT_EQ(1, relation.find(zone_a, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(1, relation.find(zone_b, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(relation.find(zone_a, ReadEventPredicate::predicate()), relation.find(zone_b, ReadEventPredicate::predicate()));
  EXPECT_EQ(0, relation.find(zone_c, ReadEventPredicate::predicate()).size());

  relation.clear();
  EXPECT_TRUE(relation.find(zone_a, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.find(zone_b, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.event_ptrs().empty());
}

TEST(RelationTest, FilterZoneRelation) {
  unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  const Zone other_zone = Zone::unique_atom();

  ZoneRelation<Event> relation;

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new DirectWriteEvent<long>(thread_id, zone, std::move(read_instr_ptr)));
  const std::shared_ptr<Event> read_event_ptr_i(new ReadEvent<int>(thread_id, zone));
  const std::shared_ptr<Event> read_event_ptr_ii(new ReadEvent<int>(thread_id, zone));

  relation.relate(write_event_ptr);
  relation.relate(read_event_ptr_i);
  relation.relate(read_event_ptr_ii);

  EXPECT_EQ(3, relation.event_ptrs().size());
  EXPECT_EQ(2, relation.find(zone, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(1, relation.find(zone, WriteEventPredicate::predicate()).size());
  EXPECT_TRUE(relation.find(other_zone, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.find(other_zone, WriteEventPredicate::predicate()).empty());

  relation.clear();
  EXPECT_TRUE(relation.find(zone, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.find(zone, WriteEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.event_ptrs().empty());
}

TEST(RelationTest, ZoneRelationWithJoin) {
  const Zone a_zone = Zone::unique_atom();
  const Zone b_zone = Zone::unique_atom();
  const Zone c_zone = a_zone.join(b_zone);

  const std::shared_ptr<Event> a_event_ptr(new TestEvent(a_zone));
  const std::shared_ptr<Event> c_event_ptr(new TestEvent(c_zone));

  ZoneRelation<Event> relation;

  relation.relate(a_event_ptr);
  relation.relate(c_event_ptr);

  EXPECT_EQ(2, relation.event_ptrs().size());
  EXPECT_EQ(2, relation.find(a_zone, ReadEventPredicate::predicate()).size());
}
