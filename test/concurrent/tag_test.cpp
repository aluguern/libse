#include "gtest/gtest.h"

#include "concurrent/tag.h"

using namespace se;

TEST(TagTest, UniqueAtom) {
  const Tag tag_a = Tag::unique_atom();
  const Tag tag_b = Tag::unique_atom();

  EXPECT_EQ(tag_a, tag_a);
  EXPECT_NE(tag_a, tag_b);
}

TEST(TagTest, AtomIsNotBottom) {
  const Tag tag = Tag::unique_atom();
  EXPECT_FALSE(tag.is_bottom());
}

TEST(TagTest, JoinBothAreAtoms) {
  const Tag tag_a = Tag::unique_atom();
  const Tag tag_b = Tag::unique_atom();

  const Tag join_tag = tag_a.join(tag_b);
  EXPECT_FALSE(join_tag.is_bottom());
}

TEST(TagTest, JoinLeftIsAtom) {
  const Tag tag_a = Tag::unique_atom();
  const Tag tag_b = Tag::bottom();

  const Tag join_tag = tag_a.join(tag_b);
  EXPECT_FALSE(join_tag.is_bottom());
}

TEST(TagTest, JoinRightIsAtom) {
  const Tag tag_a = Tag::bottom();
  const Tag tag_b = Tag::unique_atom();

  const Tag join_tag = tag_a.join(tag_b);
  EXPECT_FALSE(join_tag.is_bottom());
}

TEST(TagTest, IdempotentJoin) {
  const Tag tag = Tag::unique_atom();

  EXPECT_EQ(tag.join(tag), tag);
}

TEST(TagTest, IdempotentMeet) {
  const Tag tag = Tag::unique_atom();

  EXPECT_EQ(tag.meet(tag), tag);
}

TEST(TagTest, MeetBothAreAtoms) {
  const Tag tag_a = Tag::unique_atom();
  const Tag tag_b = Tag::unique_atom();
  const Tag tag_c = tag_a.join(tag_b);

  EXPECT_TRUE(tag_a.meet(tag_b).is_bottom());
  EXPECT_TRUE(tag_b.meet(tag_a).is_bottom());

  EXPECT_FALSE(tag_a.meet(tag_c).is_bottom());
  EXPECT_FALSE(tag_c.meet(tag_a).is_bottom());
  EXPECT_FALSE(tag_b.meet(tag_c).is_bottom());
  EXPECT_FALSE(tag_c.meet(tag_b).is_bottom());
}

TEST(TagTest, MeetRightIsAtom) {
  const Tag tag_a = Tag::bottom();
  const Tag tag_b = Tag::unique_atom();

  const Tag meet_tag = tag_a.meet(tag_b);
  EXPECT_TRUE(meet_tag.is_bottom());
}

TEST(TagTest, MeetLeftIsAtom) {
  const Tag tag_a = Tag::unique_atom();
  const Tag tag_b = Tag::bottom();

  const Tag meet_tag = tag_a.meet(tag_b);
  EXPECT_TRUE(meet_tag.is_bottom());
}

TEST(TagTest, MeetBothAreNotAtoms) {
  const Tag tag_a = Tag::bottom();
  const Tag tag_b = Tag::bottom();

  const Tag meet_tag = tag_a.meet(tag_b);
  EXPECT_TRUE(meet_tag.is_bottom());
}
