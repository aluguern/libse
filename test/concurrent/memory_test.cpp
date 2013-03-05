#include "gtest/gtest.h"

#include "concurrent/memory.h"

using namespace se;

TEST(MemoryAddrTest, Equality) {
  uintptr_t ptr = 0x03fa;

  const MemoryAddr addr_a(ptr);
  const MemoryAddr addr_b(ptr);

  EXPECT_EQ(addr_a, addr_b);
}

TEST(MemoryAddrTest, ConstructorUintptr) {
  uintptr_t ptr = 0x03fa;
  const MemoryAddr addr(ptr);

  EXPECT_EQ(1, addr.ptrs().size());
}

TEST(MemoryAddrTest, Join) {
  uintptr_t ptr_a = 0x03fa;
  uintptr_t ptr_b = 0x03fb;

  const MemoryAddr addr_a(ptr_a);
  const MemoryAddr addr_b(ptr_b);

  const MemoryAddr join_addr = MemoryAddr::join(addr_a, addr_b);
  EXPECT_EQ(2, join_addr.ptrs().size());
}
