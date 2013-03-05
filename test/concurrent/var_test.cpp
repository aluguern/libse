#include "gtest/gtest.h"

#include "concurrent/var.h"

using namespace se;

TEST(VarTest, ConcurrentVarExternalAddr) {
  Event::reset_id(7);
  const uintptr_t var_addr = 0x03fa;
  const MemoryAddr addr(var_addr);

  ConcurrentVar<char> var(addr);

  EXPECT_EQ(std::unordered_set<uintptr_t>({ var_addr }), var.addr().ptrs());

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.instr_ref());
  EXPECT_EQ(0, instr.literal());
}

TEST(VarTest, ConcurrentVarInternalAddr) {
  Event::reset_id(7);

  ConcurrentVar<char> var;

  const uintptr_t var_addr = reinterpret_cast<uintptr_t>(&var);
  EXPECT_EQ(std::unordered_set<uintptr_t>({ var_addr }), var.addr().ptrs());

  const LiteralReadInstr<char>& instr = dynamic_cast<const LiteralReadInstr<char>&>(var.instr_ref());
  EXPECT_EQ(0, instr.literal());
}
