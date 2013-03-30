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
  const MemoryAddr addr = MemoryAddr::alloc<bool>();
  const MemoryAddr offset_addr = addr + sizeof(long long);
  const MemoryAddr other_addr = MemoryAddr::alloc<long long>();

  EXPECT_EQ(other_addr, offset_addr);
}

TEST(MemoryAddrTest, DefaultIsShared) {
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  EXPECT_TRUE(addr.is_shared());
}

TEST(MemoryAddrTest, JoinBothAreNotShared) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>(false);
  const MemoryAddr addr_b = MemoryAddr::alloc<long>(false);

  const MemoryAddr join_addr = MemoryAddr::join(addr_a, addr_b);
  EXPECT_FALSE(join_addr.is_shared());
}

TEST(MemoryAddrTest, JoinLeftIsShared) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>(true);
  const MemoryAddr addr_b = MemoryAddr::alloc<long>(false);

  const MemoryAddr join_addr = MemoryAddr::join(addr_a, addr_b);
  EXPECT_TRUE(join_addr.is_shared());
}

TEST(MemoryAddrTest, JoinRightIsShared) {
  const MemoryAddr addr_a = MemoryAddr::alloc<int>(false);
  const MemoryAddr addr_b = MemoryAddr::alloc<long>(true);

  const MemoryAddr join_addr = MemoryAddr::join(addr_a, addr_b);
  EXPECT_TRUE(join_addr.is_shared());
}
