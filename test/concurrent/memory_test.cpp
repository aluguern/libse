#include "gtest/gtest.h"

#include "concurrent/memory.h"

using namespace se;

TEST(MemoryAddrTest, UniqueAddr) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>();
  const MemoryAddr addr_b = MemoryAddr::alloc<int>();

  EXPECT_TRUE(addr_a == addr_a);
  EXPECT_FALSE(addr_a == addr_b);
}

TEST(MemoryAddrTest, Offset) {
  const MemoryAddr addr = MemoryAddr::alloc<long long>();
  const MemoryAddr offset_addr = addr + sizeof(long long);
  const MemoryAddr other_addr = MemoryAddr::alloc<bool>();

  EXPECT_EQ(other_addr, offset_addr);
}

TEST(MemoryAddrTest, OffsetOnRegion) {
  static_assert(1 == sizeof(bool), "Test assumption fails");

  constexpr size_t MEM_BEGIN = 2;
  constexpr size_t N = 5;

  MemoryAddr::reset(MEM_BEGIN);
  const MemoryAddr addr = MemoryAddr::alloc<int>(true, N);

  MemoryAddr::reset(MEM_BEGIN);
  const MemoryAddr addr_a = MemoryAddr::alloc<bool>();
  const MemoryAddr addr_b = MemoryAddr::alloc<bool>();
  const MemoryAddr addr_c = MemoryAddr::alloc<bool>();
  const MemoryAddr addr_d = MemoryAddr::alloc<bool>();
  const MemoryAddr addr_e = MemoryAddr::alloc<bool>();

  EXPECT_NE(addr_a, addr);
  EXPECT_NE(addr_b, addr + 1);
  EXPECT_NE(addr_c, addr + 2);
  EXPECT_NE(addr_d, addr + 3);
  EXPECT_NE(addr_e, addr + 4);
}

TEST(MemoryAddrTest, OffsetOnBase) {
  static_assert(1 == sizeof(bool), "Test assumption fails");

  constexpr size_t MEM_BEGIN = 2;
  constexpr size_t N = 5;

  MemoryAddr::reset(MEM_BEGIN);
  const MemoryAddr addr = MemoryAddr::alloc<int[N]>(true, 1);

  MemoryAddr::reset(MEM_BEGIN);
  const MemoryAddr addr_a = MemoryAddr::alloc<bool>();
  const MemoryAddr addr_b = MemoryAddr::alloc<bool>();
  const MemoryAddr addr_c = MemoryAddr::alloc<bool>();
  const MemoryAddr addr_d = MemoryAddr::alloc<bool>();
  const MemoryAddr addr_e = MemoryAddr::alloc<bool>();

  EXPECT_EQ(addr_a, addr);
  EXPECT_EQ(addr_b, addr + 1);
  EXPECT_EQ(addr_c, addr + 2);
  EXPECT_EQ(addr_d, addr + 3);
  EXPECT_EQ(addr_e, addr + 4);
}

TEST(MemoryAddrTest, ArrayAddr) {
  constexpr size_t MEM_BEGIN = 2;
  constexpr size_t N = 5;

  MemoryAddr::reset(MEM_BEGIN);
  const MemoryAddr addr_a = MemoryAddr::alloc<int>(true, N);
  const MemoryAddr addr_b = MemoryAddr::alloc<bool>(true);

  MemoryAddr::reset(MEM_BEGIN);
  const MemoryAddr addr_x = MemoryAddr::alloc<int[N]>(true);
  const MemoryAddr addr_y = MemoryAddr::alloc<bool>(true);

  EXPECT_NE(addr_a, addr_x);
  EXPECT_EQ(addr_b, addr_y);
}

TEST(MemoryAddrTest, DefaultIsShared) {
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  EXPECT_TRUE(addr.is_shared());
  EXPECT_FALSE(addr.is_bottom());
}

TEST(MemoryAddrTest, JoinBothAreNotShared) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>(false);
  const MemoryAddr addr_b = MemoryAddr::alloc<long>(false);

  const MemoryAddr join_addr = addr_a.join(addr_b);
  EXPECT_FALSE(join_addr.is_shared());
  EXPECT_FALSE(join_addr.is_bottom());
}

TEST(MemoryAddrTest, JoinLeftIsShared) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>(true);
  const MemoryAddr addr_b = MemoryAddr::alloc<long>(false);

  const MemoryAddr join_addr = addr_a.join(addr_b);
  EXPECT_TRUE(join_addr.is_shared());
  EXPECT_FALSE(join_addr.is_bottom());
}

TEST(MemoryAddrTest, JoinRightIsShared) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>(false);
  const MemoryAddr addr_b = MemoryAddr::alloc<long>(true);

  const MemoryAddr join_addr = addr_a.join(addr_b);
  EXPECT_TRUE(join_addr.is_shared());
  EXPECT_FALSE(join_addr.is_bottom());
}

TEST(MemoryAddrTest, MeetBothAreShared) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>(true);
  const MemoryAddr addr_b = MemoryAddr::alloc<long>(true);
  const MemoryAddr addr_c = addr_a.join(addr_b);

  EXPECT_TRUE(addr_a.meet(addr_b).is_bottom());
  EXPECT_TRUE(addr_a.meet(addr_b).is_shared());

  EXPECT_TRUE(addr_b.meet(addr_a).is_bottom());
  EXPECT_TRUE(addr_b.meet(addr_a).is_shared());

  EXPECT_FALSE(addr_a.meet(addr_c).is_bottom());
  EXPECT_TRUE(addr_a.meet(addr_c).is_shared());

  EXPECT_FALSE(addr_c.meet(addr_a).is_bottom());
  EXPECT_TRUE(addr_c.meet(addr_a).is_shared());

  EXPECT_FALSE(addr_b.meet(addr_c).is_bottom());
  EXPECT_TRUE(addr_b.meet(addr_c).is_shared());

  EXPECT_FALSE(addr_c.meet(addr_b).is_bottom());
  EXPECT_TRUE(addr_c.meet(addr_b).is_shared());
}

TEST(MemoryAddrTest, MeetRightIsShared) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>(false);
  const MemoryAddr addr_b = MemoryAddr::alloc<long>(true);

  const MemoryAddr meet_addr = addr_a.meet(addr_b);
  EXPECT_FALSE(meet_addr.is_shared());
  EXPECT_TRUE(meet_addr.is_bottom());
}

TEST(MemoryAddrTest, MeetLeftIsShared) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>(true);
  const MemoryAddr addr_b = MemoryAddr::alloc<long>(false);

  const MemoryAddr meet_addr = addr_a.meet(addr_b);
  EXPECT_FALSE(meet_addr.is_shared());
  EXPECT_TRUE(meet_addr.is_bottom());
}

TEST(MemoryAddrTest, MeetBothAreNotShared) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>(false);
  const MemoryAddr addr_b = MemoryAddr::alloc<long>(false);

  const MemoryAddr meet_addr = addr_a.meet(addr_b);
  EXPECT_FALSE(meet_addr.is_shared());
  EXPECT_TRUE(meet_addr.is_bottom());
}
