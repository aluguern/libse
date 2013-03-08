#include "gtest/gtest.h"

#include "concurrent/relation.h"
#include "concurrent/memory.h"

using namespace se;

/// A read event on an integer
class TestEvent : public Event {
public:
  TestEvent(const MemoryAddr& addr,
    const std::shared_ptr<ReadInstr<bool>>& condition_ptr = nullptr) :
    Event(0, addr, true, &TypeInfo<int>::s_type, condition_ptr) {}
};

TEST(RelationTest, DefaultMemoryAccessRelation) {
  uintptr_t ptr = 0x03fa;

  const std::shared_ptr<TestEvent> event_ptr(new TestEvent(ptr));

  MemoryAccessRelation<TestEvent> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.find(ptr).size());
  EXPECT_EQ(0, relation.find(0x02e9).size());
}

TEST(RelationTest, MemoryAccessFunction) {
  uintptr_t ptr = 0x03fa;

  const std::shared_ptr<TestEvent> event_ptr(new TestEvent(ptr));

  MemoryAccessRelation<TestEvent> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.find(ptr).size());
  EXPECT_EQ(0, relation.find(0x02e9).size());
}

TEST(RelationTest, MemoryAccessRelation) {
  uintptr_t ptr_a = 0x03fa;
  uintptr_t ptr_b = 0x03fb;
  const MemoryAddr addr = MemoryAddr::join(ptr_a, ptr_b);

  const std::shared_ptr<TestEvent> event_ptr(new TestEvent(addr));

  MemoryAccessRelation<TestEvent> relation;

  relation.relate(event_ptr);

  EXPECT_EQ(1, relation.find(ptr_a).size());
  EXPECT_EQ(1, relation.find(ptr_b).size());
  EXPECT_EQ(relation.find(ptr_a), relation.find(ptr_b));
  EXPECT_EQ(0, relation.find(0x02e9).size());
}
