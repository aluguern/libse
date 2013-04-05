#include "gtest/gtest.h"

#include "concurrent/relation.h"
#include "concurrent/tag.h"
#include "concurrent/instr.h"
#include "concurrent/encoder.h"

using namespace se;

/// A read event on an integer
class TestEvent : public Event {
public:
  TestEvent(const Tag& tag,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(0, tag, true, &TypeInfo<int>::s_type, condition_ptr) {}

  z3::expr encode_eq(const Z3ValueEncoder& encoder, Z3& helper) const {
    return helper.constant(*this);
  }

  z3::expr constant(Z3& helper) const { return helper.constant(*this); }
};


TEST(RelationTest, ReadEventPredicate) {
  const unsigned thread_id = 3;
  const Tag tag = Tag::unique_atom();

  const Predicate<std::shared_ptr<Event>>& predicate(ReadEventPredicate::predicate());

  const std::shared_ptr<Event> read_event_ptr(new ReadEvent<int>(thread_id, tag));
  EXPECT_TRUE(predicate.check(read_event_ptr));

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new DirectWriteEvent<long>(thread_id, tag, std::move(read_instr_ptr)));
  EXPECT_FALSE(predicate.check(write_event_ptr));
}

TEST(RelationTest, WriteEventPredicate) {
  const unsigned thread_id = 3;
  const Tag tag = Tag::unique_atom();

  const Predicate<std::shared_ptr<Event>>& predicate(WriteEventPredicate::predicate());

  const std::shared_ptr<Event> read_event_ptr(new ReadEvent<int>(thread_id, tag));
  EXPECT_FALSE(predicate.check(read_event_ptr));

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new DirectWriteEvent<long>(thread_id, tag, std::move(read_instr_ptr)));
  EXPECT_TRUE(predicate.check(write_event_ptr));
}

TEST(RelationTest, DefaultTagRelation) {
  const Tag tag = Tag::unique_atom();
  const Tag other_tag = Tag::unique_atom();

  const std::shared_ptr<Event> event_ptr(new TestEvent(tag));

  TagRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.event_ptrs().size());
  EXPECT_EQ(1, relation.find(tag, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(0, relation.find(other_tag, ReadEventPredicate::predicate()).size());

  relation.clear();
  EXPECT_TRUE(relation.find(tag, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.event_ptrs().empty());
}

TEST(RelationTest, TagRelationAsFunction) {
  const Tag tag = Tag::unique_atom();
  const Tag other_tag = Tag::unique_atom();

  const std::shared_ptr<Event> event_ptr(new TestEvent(tag));

  TagRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.event_ptrs().size());
  EXPECT_EQ(1, relation.find(tag, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(0, relation.find(other_tag, ReadEventPredicate::predicate()).size());

  relation.clear();
  EXPECT_TRUE(relation.find(tag, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.event_ptrs().empty());
}

TEST(RelationTest, TagRelation) {
  const Tag tag_a = Tag::unique_atom();
  const Tag tag_b = Tag::unique_atom();
  const Tag tag_c = Tag::unique_atom();
  const Tag tag = tag_a.join(tag_b);

  const std::shared_ptr<Event> event_ptr(new TestEvent(tag));

  TagRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.event_ptrs().size());
  EXPECT_EQ(1, relation.find(tag_a, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(1, relation.find(tag_b, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(relation.find(tag_a, ReadEventPredicate::predicate()), relation.find(tag_b, ReadEventPredicate::predicate()));
  EXPECT_EQ(0, relation.find(tag_c, ReadEventPredicate::predicate()).size());

  relation.clear();
  EXPECT_TRUE(relation.find(tag_a, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.find(tag_b, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.event_ptrs().empty());
}

TEST(RelationTest, FilterTagRelation) {
  unsigned thread_id = 3;
  const Tag tag = Tag::unique_atom();
  const Tag other_tag = Tag::unique_atom();

  TagRelation<Event> relation;

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new DirectWriteEvent<long>(thread_id, tag, std::move(read_instr_ptr)));
  const std::shared_ptr<Event> read_event_ptr_i(new ReadEvent<int>(thread_id, tag));
  const std::shared_ptr<Event> read_event_ptr_ii(new ReadEvent<int>(thread_id, tag));

  relation.relate(write_event_ptr);
  relation.relate(read_event_ptr_i);
  relation.relate(read_event_ptr_ii);

  EXPECT_EQ(3, relation.event_ptrs().size());
  EXPECT_EQ(2, relation.find(tag, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(1, relation.find(tag, WriteEventPredicate::predicate()).size());
  EXPECT_TRUE(relation.find(other_tag, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.find(other_tag, WriteEventPredicate::predicate()).empty());

  relation.clear();
  EXPECT_TRUE(relation.find(tag, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.find(tag, WriteEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.event_ptrs().empty());
}

TEST(RelationTest, TagRelationWithJoin) {
  const Tag a_tag = Tag::unique_atom();
  const Tag b_tag = Tag::unique_atom();
  const Tag c_tag = a_tag.join(b_tag);

  const std::shared_ptr<Event> a_event_ptr(new TestEvent(a_tag));
  const std::shared_ptr<Event> c_event_ptr(new TestEvent(c_tag));

  TagRelation<Event> relation;

  relation.relate(a_event_ptr);
  relation.relate(c_event_ptr);

  EXPECT_EQ(2, relation.event_ptrs().size());
  EXPECT_EQ(2, relation.find(a_tag, ReadEventPredicate::predicate()).size());
}
