#include "gtest/gtest.h"

#include "concurrent/relation.h"
#include "concurrent/memory.h"
#include "concurrent/instr.h"

using namespace se;

/// A read event on an integer
class TestEvent : public Event {
public:
  TestEvent(const MemoryAddr& addr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(0, addr, true, &TypeInfo<int>::s_type, condition_ptr) {}
};


TEST(RelationTest, ReadEventPredicate) {
  unsigned thread_id = 3;
  const uintptr_t ptr = 0x03fa;

  const Predicate<std::shared_ptr<Event>>& predicate(ReadEventPredicate::predicate());

  const std::shared_ptr<Event> read_event_ptr(new ReadEvent<int>(thread_id, ptr));
  EXPECT_TRUE(predicate.check(read_event_ptr));

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new WriteEvent<long>(thread_id, ptr, std::move(read_instr_ptr)));
  EXPECT_FALSE(predicate.check(write_event_ptr));
}

TEST(RelationTest, WriteEventPredicate) {
  unsigned thread_id = 3;
  const uintptr_t ptr = 0x03fa;

  const Predicate<std::shared_ptr<Event>>& predicate(WriteEventPredicate::predicate());

  const std::shared_ptr<Event> read_event_ptr(new ReadEvent<int>(thread_id, ptr));
  EXPECT_FALSE(predicate.check(read_event_ptr));

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new WriteEvent<long>(thread_id, ptr, std::move(read_instr_ptr)));
  EXPECT_TRUE(predicate.check(write_event_ptr));
}

TEST(RelationTest, DefaultMemoryAccessRelation) {
  uintptr_t ptr = 0x03fa;

  const std::shared_ptr<Event> event_ptr(new TestEvent(ptr));

  MemoryAccessRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.find(ptr, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(0, relation.find(0x02e9, ReadEventPredicate::predicate()).size());
}

TEST(RelationTest, MemoryAccessFunction) {
  uintptr_t ptr = 0x03fa;

  const std::shared_ptr<Event> event_ptr(new TestEvent(ptr));

  MemoryAccessRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.find(ptr, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(0, relation.find(0x02e9, ReadEventPredicate::predicate()).size());
}

TEST(RelationTest, MemoryAccessRelation) {
  uintptr_t ptr_a = 0x03fa;
  uintptr_t ptr_b = 0x03fb;
  const MemoryAddr addr = MemoryAddr::join(ptr_a, ptr_b);

  const std::shared_ptr<Event> event_ptr(new TestEvent(addr));

  MemoryAccessRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.find(ptr_a, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(1, relation.find(ptr_b, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(relation.find(ptr_a, ReadEventPredicate::predicate()), relation.find(ptr_b, ReadEventPredicate::predicate()));
  EXPECT_EQ(0, relation.find(0x02e9, ReadEventPredicate::predicate()).size());
}

TEST(RelationTest, FilterMemoryAccessRelation) {
  unsigned thread_id = 3;
  uintptr_t ptr = 0x03fa;

  MemoryAccessRelation<Event> relation;

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new WriteEvent<long>(thread_id, ptr, std::move(read_instr_ptr)));
  const std::shared_ptr<Event> read_event_ptr_i(new ReadEvent<int>(thread_id, ptr));
  const std::shared_ptr<Event> read_event_ptr_ii(new ReadEvent<int>(thread_id, ptr));

  relation.relate(write_event_ptr);
  relation.relate(read_event_ptr_i);
  relation.relate(read_event_ptr_ii);

  EXPECT_EQ(2, relation.find(ptr, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(1, relation.find(ptr, WriteEventPredicate::predicate()).size());
  EXPECT_TRUE(relation.find(0x02e9, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.find(0x02e9, WriteEventPredicate::predicate()).empty());
}
