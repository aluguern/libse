#include "concurrent/zone.h"
#include "gtest/gtest.h"

using namespace se;

TEST(ZoneTest, UniqueAtom) {
  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = Zone::unique_atom();

  EXPECT_EQ(zone_a, zone_a);
  EXPECT_NE(zone_a, zone_b);
}

TEST(ZoneTest, AtomIsNotBottom) {
  const Zone zone = Zone::unique_atom();
  EXPECT_FALSE(zone.is_bottom());
}

TEST(ZoneTest, JoinBothAreAtoms) {
  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = Zone::unique_atom();

  const Zone join_zone = zone_a.join(zone_b);
  EXPECT_FALSE(join_zone.is_bottom());
}

TEST(ZoneTest, JoinLeftIsAtom) {
  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = Zone::bottom();

  const Zone join_zone = zone_a.join(zone_b);
  EXPECT_FALSE(join_zone.is_bottom());
}

TEST(ZoneTest, JoinRightIsAtom) {
  const Zone zone_a = Zone::bottom();
  const Zone zone_b = Zone::unique_atom();

  const Zone join_zone = zone_a.join(zone_b);
  EXPECT_FALSE(join_zone.is_bottom());
}

TEST(ZoneTest, IdempotentJoin) {
  const Zone zone = Zone::unique_atom();

  EXPECT_EQ(zone.join(zone), zone);
}

TEST(ZoneTest, IdempotentMeet) {
  const Zone zone = Zone::unique_atom();

  EXPECT_EQ(zone.meet(zone), zone);
}

TEST(ZoneTest, MeetBothAreAtoms) {
  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = Zone::unique_atom();
  const Zone zone_c = zone_a.join(zone_b);

  EXPECT_TRUE(zone_a.meet(zone_b).is_bottom());
  EXPECT_TRUE(zone_b.meet(zone_a).is_bottom());

  EXPECT_FALSE(zone_a.meet(zone_c).is_bottom());
  EXPECT_FALSE(zone_c.meet(zone_a).is_bottom());
  EXPECT_FALSE(zone_b.meet(zone_c).is_bottom());
  EXPECT_FALSE(zone_c.meet(zone_b).is_bottom());
}

TEST(ZoneTest, MeetRightIsAtom) {
  const Zone zone_a = Zone::bottom();
  const Zone zone_b = Zone::unique_atom();

  const Zone meet_zone = zone_a.meet(zone_b);
  EXPECT_TRUE(meet_zone.is_bottom());
}

TEST(ZoneTest, MeetLeftIsAtom) {
  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = Zone::bottom();

  const Zone meet_zone = zone_a.meet(zone_b);
  EXPECT_TRUE(meet_zone.is_bottom());
}

TEST(ZoneTest, MeetBothAreNotAtoms) {
  const Zone zone_a = Zone::bottom();
  const Zone zone_b = Zone::bottom();

  const Zone meet_zone = zone_a.meet(zone_b);
  EXPECT_TRUE(meet_zone.is_bottom());
}

TEST(ZoneTest, Copy) {
  const Zone zone_a = Zone::unique_atom();
  const Zone zone_b = zone_a;

  EXPECT_EQ(zone_a, zone_b);
}
