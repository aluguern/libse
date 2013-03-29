#include "gtest/gtest.h"

#include "concurrent/relation.h"
#include "concurrent/memory.h"
#include "concurrent/instr.h"
#include "concurrent/encoder.h"

using namespace se;

/// A read event on an integer
class TestEvent : public Event {
public:
  TestEvent(const MemoryAddr& addr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(0, addr, true, &TypeInfo<int>::s_type, condition_ptr) {}

  z3::expr encode(const Z3ValueEncoder& encoder, Z3& helper) const {
    return helper.constant(*this);
  }
};


TEST(RelationTest, ReadEventPredicate) {
  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int>();

  const Predicate<std::shared_ptr<Event>>& predicate(ReadEventPredicate::predicate());

  const std::shared_ptr<Event> read_event_ptr(new ReadEvent<int>(thread_id, addr));
  EXPECT_TRUE(predicate.check(read_event_ptr));

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new DirectWriteEvent<long>(thread_id, addr, std::move(read_instr_ptr)));
  EXPECT_FALSE(predicate.check(write_event_ptr));
}

TEST(RelationTest, WriteEventPredicate) {
  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int>();

  const Predicate<std::shared_ptr<Event>>& predicate(WriteEventPredicate::predicate());

  const std::shared_ptr<Event> read_event_ptr(new ReadEvent<int>(thread_id, addr));
  EXPECT_FALSE(predicate.check(read_event_ptr));

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new DirectWriteEvent<long>(thread_id, addr, std::move(read_instr_ptr)));
  EXPECT_TRUE(predicate.check(write_event_ptr));
}

TEST(RelationTest, DefaultMemoryAddrRelation) {
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  const MemoryAddr other_addr = MemoryAddr::alloc<int>();

  const std::shared_ptr<Event> event_ptr(new TestEvent(addr));

  MemoryAddrRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.find(addr, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(0, relation.find(other_addr, ReadEventPredicate::predicate()).size());

  relation.clear();
  EXPECT_TRUE(relation.find(addr, ReadEventPredicate::predicate()).empty());
}

TEST(RelationTest, MemoryAddrRelationAsFunction) {
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  const MemoryAddr other_addr = MemoryAddr::alloc<int>();

  const std::shared_ptr<Event> event_ptr(new TestEvent(addr));

  MemoryAddrRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.find(addr, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(0, relation.find(other_addr, ReadEventPredicate::predicate()).size());

  relation.clear();
  EXPECT_TRUE(relation.find(addr, ReadEventPredicate::predicate()).empty());
}

TEST(RelationTest, MemoryAddrRelation) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>();
  const MemoryAddr addr_b = MemoryAddr::alloc<int>();
  const MemoryAddr addr_c = MemoryAddr::alloc<int>();
  const MemoryAddr addr = MemoryAddr::join(addr_a, addr_b);

  const std::shared_ptr<Event> event_ptr(new TestEvent(addr));

  MemoryAddrRelation<Event> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.find(addr_a, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(1, relation.find(addr_b, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(relation.find(addr_a, ReadEventPredicate::predicate()), relation.find(addr_b, ReadEventPredicate::predicate()));
  EXPECT_EQ(0, relation.find(addr_c, ReadEventPredicate::predicate()).size());

  relation.clear();
  EXPECT_TRUE(relation.find(addr_a, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.find(addr_b, ReadEventPredicate::predicate()).empty());
}

TEST(RelationTest, FilterMemoryAddrRelation) {
  unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  const MemoryAddr other_addr = MemoryAddr::alloc<int>();

  MemoryAddrRelation<Event> relation;

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));
  const std::shared_ptr<Event> write_event_ptr(new DirectWriteEvent<long>(thread_id, addr, std::move(read_instr_ptr)));
  const std::shared_ptr<Event> read_event_ptr_i(new ReadEvent<int>(thread_id, addr));
  const std::shared_ptr<Event> read_event_ptr_ii(new ReadEvent<int>(thread_id, addr));

  relation.relate(write_event_ptr);
  relation.relate(read_event_ptr_i);
  relation.relate(read_event_ptr_ii);

  EXPECT_EQ(2, relation.find(addr, ReadEventPredicate::predicate()).size());
  EXPECT_EQ(1, relation.find(addr, WriteEventPredicate::predicate()).size());
  EXPECT_TRUE(relation.find(other_addr, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.find(other_addr, WriteEventPredicate::predicate()).empty());

  relation.clear();
  EXPECT_TRUE(relation.find(addr, ReadEventPredicate::predicate()).empty());
  EXPECT_TRUE(relation.find(addr, WriteEventPredicate::predicate()).empty());
}
