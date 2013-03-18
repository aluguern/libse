#include "gtest/gtest.h"

#include <limits>

#include "concurrent/encoder.h"

using namespace se;

TEST(EncoderTest, Z3BvLiteralBool) {
  Z3 z3;

  const bool literal = true;
  const LiteralReadInstr<bool> instr(literal);

  z3.solver.push();

  EXPECT_TRUE(z3.literal(instr).is_bool());

  z3.solver.add(!z3.literal(instr));
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3BvLiteral) {
  Z3 z3;

  const short literal = 3;
  const LiteralReadInstr<short> instr(literal);

  EXPECT_TRUE(z3.literal(instr).is_bv());
  EXPECT_EQ(TypeInfo<short>::s_type.bv_size(),
    z3.literal(instr).get_sort().bv_size());

  z3.solver.push();

  z3.solver.add(z3.literal(instr) != literal);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  // Sanity check satisfiable formula
  z3.solver.add(z3.literal(instr) == literal);
  EXPECT_EQ(z3::sat, z3.solver.check());
}

TEST(EncoderTest, Z3BvConstant) {
  Z3 z3;

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  const ReadEvent<int> event(thread_id, addr);

  EXPECT_TRUE(z3.constant(event).is_bv());
  EXPECT_EQ(TypeInfo<int>::s_type.bv_size(),
    z3.constant(event).get_sort().bv_size());

  z3.solver.add(z3.constant(event) != z3.constant(event));

  // Proves that both bit vector constants are equal
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3BoolConstant) {
  Z3 z3;

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<bool>();
  const ReadEvent<bool> event(thread_id, addr);

  EXPECT_TRUE(z3.constant(event).is_bool());

  z3.solver.add(z3.constant(event) != z3.constant(event));

  // Proves that both Boolean constants are equal
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3ArrayConstant) {
  Z3 z3;

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int[5]>();
  const ReadEvent<int[5]> read_event(thread_id, addr);

  const z3::expr event_expr(z3.constant(read_event));
  EXPECT_TRUE(event_expr.is_array());

  EXPECT_TRUE(event_expr.get_sort().array_range().is_bv());
  EXPECT_EQ(TypeInfo<size_t>::s_type.bv_size(),
    event_expr.get_sort().array_domain().bv_size());

  EXPECT_TRUE(event_expr.get_sort().array_domain().is_bv());
  EXPECT_EQ(TypeInfo<int>::s_type.bv_size(),
    event_expr.get_sort().array_range().bv_size());

  z3.solver.push();

  z3.solver.add(event_expr != z3.constant(read_event));

  // Proves that both bit vector constants are equal
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  // Sanity checks ...

  // 1:
  z3.solver.push();

  z3.solver.add(event_expr == z3.constant(read_event));
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  // 2:
  z3.solver.push();

  const int index = 3;
  const int value_a = 7;
  const int value_b = 8;
  z3::expr array_a(z3::store(event_expr, index, value_a));
  z3::expr array_b(z3::store(z3.constant(read_event), index, value_b));

  z3.solver.add(array_a == array_b);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  // 3:
  z3.solver.push();

  z3.solver.add(array_a != array_b);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  // 4:
  z3.solver.push();

  z3.solver.add(z3.constant(read_event) == array_b);
  z3.solver.add(z3::select(event_expr, index) == value_b);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  // 5:
  z3.solver.push();

  z3.solver.add(z3.constant(read_event) == array_b);
  z3.solver.add(z3::select(event_expr, index) != value_b);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(EncoderTest, ReadFromSymbol) {
  const unsigned write_thread_id = 7;
  const unsigned read_thread_id = 8;

  Z3 z3;

  const MemoryAddr addr = MemoryAddr::alloc<short>();
  std::unique_ptr<ReadInstr<short>> instr_ptr(new LiteralReadInstr<short>(5));
  const DirectWriteEvent<short> write_event(write_thread_id, addr, std::move(instr_ptr));
  const ReadEvent<short> read_event(read_thread_id, addr);

  EXPECT_TRUE(z3.constant(write_event, read_event).is_bool());
}

TEST(EncoderTest, Z3IndirectWriteEventConstant) {
  const unsigned thread_id = 3;
  const size_t array_size = 5;

  Z3 z3;

  const MemoryAddr pointer_addr = MemoryAddr::alloc<char[array_size]>();
  std::unique_ptr<ReadEvent<char[array_size]>> pointer_event_ptr(new ReadEvent<char[array_size]>(thread_id, pointer_addr));
  std::unique_ptr<ReadInstr<char[array_size]>> pointer_read_instr(new BasicReadInstr<char[array_size]>(std::move(pointer_event_ptr)));

  std::unique_ptr<ReadInstr<size_t>> offset_read_instr(new LiteralReadInstr<size_t>(7));

  std::unique_ptr<DerefReadInstr<char[array_size], size_t>> deref_read_instr_ptr(
    new DerefReadInstr<char[array_size], size_t>(
      std::move(pointer_read_instr), std::move(offset_read_instr)));

  std::unique_ptr<ReadInstr<char>> read_instr_ptr(new LiteralReadInstr<char>('X'));

  const MemoryAddr write_addr = MemoryAddr::alloc<char>();
  const IndirectWriteEvent<char, size_t, array_size> write_event(thread_id, write_addr,
    std::move(deref_read_instr_ptr), std::move(read_instr_ptr));

  const z3::expr event_expr(z3.constant(write_event));
  EXPECT_TRUE(event_expr.is_array());

  EXPECT_TRUE(event_expr.get_sort().array_range().is_bv());
  EXPECT_EQ(TypeInfo<size_t>::s_type.bv_size(),
    event_expr.get_sort().array_domain().bv_size());

  EXPECT_TRUE(event_expr.get_sort().array_domain().is_bv());
  EXPECT_EQ(TypeInfo<char>::s_type.bv_size(),
    event_expr.get_sort().array_range().bv_size());
}

TEST(EncoderTest, Z3Clock) {
  Z3 z3;

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  const ReadEvent<int> event(thread_id, addr);

  z3.solver.push();
  z3.solver.add(z3.clock(event) <= 0);

  // Proves that clock values are natural numbers
  EXPECT_EQ(z3::unsat, z3.solver.check());

  // Sanity check a satisfiable formula
  z3.solver.pop();
  z3.solver.add(z3.clock(event) <= 1);
  EXPECT_EQ(z3::sat, z3.solver.check());
}

TEST(EncoderTest, Z3ReadEncoderForLiteralReadInstr) {
  const Z3ReadEncoder encoder;
  Z3 z3;

  const short literal = 3;
  const LiteralReadInstr<short> instr(literal);

  z3.solver.push();

  z3.solver.add(encoder.encode(instr, z3) != literal);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(encoder.encode(instr, z3) != z3.literal(instr));
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  // Sanity check satisfiable formula
  z3.solver.add(encoder.encode(instr, z3) == literal);
  EXPECT_EQ(z3::sat, z3.solver.check());
}

TEST(EncoderTest, Z3ReadEncoderForBasicReadInstrAsBv) {
  const Z3ReadEncoder encoder;
  Z3 z3;

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, addr));
  const BasicReadInstr<int> instr(std::move(event_ptr));

  z3.solver.add(encoder.encode(instr, z3) != z3.constant(*instr.event_ptr()));

  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3ReadEncoderForBasicReadInstrAsArray) {
  const Z3ReadEncoder encoder;
  Z3 z3;

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<char[5]>();
  std::unique_ptr<ReadEvent<char[5]>> event_ptr(new ReadEvent<char[5]>(thread_id, addr));
  const BasicReadInstr<char[5]> instr(std::move(event_ptr));

  // See also Z3ArrayConstant test
  z3.solver.add(encoder.encode(instr, z3) != z3.constant(*instr.event_ptr()));
  EXPECT_EQ(z3::unsat, z3.solver.check());

  const z3::expr& expr = encoder.encode(instr, z3);

  EXPECT_TRUE(expr.is_array());

  EXPECT_TRUE(expr.get_sort().array_domain().is_bv());
  EXPECT_EQ(TypeInfo<size_t>::s_type.bv_size(),
    expr.get_sort().array_domain().bv_size());

  EXPECT_TRUE(expr.get_sort().array_range().is_bv());
  EXPECT_EQ(TypeInfo<char>::s_type.bv_size(),
    expr.get_sort().array_range().bv_size());
}

TEST(EncoderTest, Z3ReadEncoderForUnaryReadInstrAsLiteralBool) {
  const Z3ReadEncoder encoder;
  Z3 z3;

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<int>();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, addr));
  std::unique_ptr<ReadInstr<bool>> instr_ptr(new LiteralReadInstr<bool>(false));
  const UnaryReadInstr<NOT, bool> instr(std::move(instr_ptr));

  EXPECT_TRUE(encoder.encode(instr, z3).is_bool());
  z3.solver.add(!encoder.encode(instr, z3));

  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3ReadEncoderForUnaryReadInstrAsLiteralInteger) {
  const Z3ReadEncoder encoder;
  Z3 z3;

  std::unique_ptr<ReadInstr<short>> instr_ptr(new LiteralReadInstr<short>(7));
  const UnaryReadInstr<SUB, short> instr(std::move(instr_ptr));

  EXPECT_TRUE(encoder.encode(instr, z3).is_bv());
  z3.solver.add(encoder.encode(instr, z3) != -7);

  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3ReadEncoderForUnaryReadInstrAsInteger) {
  const Z3ReadEncoder encoder;
  Z3 z3;

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<short>();
  std::unique_ptr<ReadEvent<short>> event_ptr(new ReadEvent<short>(thread_id, addr));
  std::unique_ptr<ReadInstr<short>> instr_ptr(new BasicReadInstr<short>(std::move(event_ptr)));
  const UnaryReadInstr<SUB, short> instr(std::move(instr_ptr));

  EXPECT_TRUE(encoder.encode(instr, z3).is_bv());

  z3.solver.push();

  z3.solver.add(encoder.encode(instr, z3) != -instr.operand_ref().encode(encoder, z3));
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.add(instr.operand_ref().encode(encoder, z3) < 0);
  z3.solver.add(encoder.encode(instr, z3) < 0);

  // Finds an overflow to satisfy the formulas
  EXPECT_EQ(z3::sat, z3.solver.check());

  // Since we prevent overflows, the assertions are now unsatisfiable.
  z3.solver.add(instr.operand_ref().encode(encoder, z3) > std::numeric_limits<short>::min());
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3ReadEncoderForBinaryReadInstrAsBool) {
  const Z3ReadEncoder encoder;
  Z3 z3;

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<short>();
  std::unique_ptr<ReadEvent<short>> levent_ptr(new ReadEvent<short>(thread_id, addr));
  std::unique_ptr<ReadInstr<short>> linstr_ptr(new BasicReadInstr<short>(std::move(levent_ptr)));

  std::unique_ptr<ReadInstr<short>> rinstr_ptr(new LiteralReadInstr<short>(7));

  const BinaryReadInstr<LSS, short, short> instr(std::move(linstr_ptr), std::move(rinstr_ptr));

  EXPECT_TRUE(encoder.encode(instr, z3).is_bool());
  z3.solver.add(encoder.encode(instr, z3));

  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.add(instr.loperand_ref().encode(encoder, z3) >= 7);

  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3ReadEncoderForBinaryReadInstrAsInteger) {
  const Z3ReadEncoder encoder;
  Z3 z3;

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<short>();
  std::unique_ptr<ReadEvent<short>> levent_ptr(new ReadEvent<short>(thread_id, addr));
  std::unique_ptr<ReadInstr<short>> linstr_ptr(new BasicReadInstr<short>(std::move(levent_ptr)));

  std::unique_ptr<ReadInstr<short>> rinstr_ptr(new LiteralReadInstr<short>(7));

  const BinaryReadInstr<ADD, short, short> instr(std::move(linstr_ptr), std::move(rinstr_ptr));

  EXPECT_TRUE(encoder.encode(instr, z3).is_bv());
  z3.solver.add(encoder.encode(instr, z3) <= instr.loperand_ref().encode(encoder, z3));

  // Finds an overflow to satisfy the formulas
  EXPECT_EQ(z3::sat, z3.solver.check());

  const short upper_bound = std::numeric_limits<short>::max() - 6;
  z3.solver.add(instr.loperand_ref().encode(encoder, z3) < upper_bound);

  // Since we prevent overflows, the assertions are now unsatisfiable.
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3ReadEncoderForDerefReadInstrAsInteger) {
  const Z3ReadEncoder encoder;
  Z3 z3;

  const unsigned thread_id = 3;
  const MemoryAddr addr = MemoryAddr::alloc<short>();
  std::unique_ptr<ReadEvent<short[5]>> event_ptr(new ReadEvent<short[5]>(thread_id, addr));
  std::unique_ptr<ReadInstr<short[5]>> memory_ptr(new BasicReadInstr<short[5]>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<size_t>> offset_ptr(
    new LiteralReadInstr<size_t>(/* out of bounds allowed for now */ 7));

  const DerefReadInstr<short[5], size_t> instr(std::move(memory_ptr), std::move(offset_ptr));

  // See also Z3ReadEncoderForBasicReadInstrAsArray test
  const BasicReadInstr<short[5]>& array_read_instr =
    static_cast<const BasicReadInstr<short[5]>&>(instr.memory_ref());
  EXPECT_TRUE(array_read_instr.encode(encoder, z3).is_array());
}

TEST(EncoderTest, Z3ValueEncoderDirectWriteEvent) {
  const unsigned thread_id = 3;

  const Z3ValueEncoder encoder;
  Z3 z3;

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));

  const MemoryAddr write_addr = MemoryAddr::alloc<long>();
  const DirectWriteEvent<long> write_event(thread_id, write_addr, std::move(read_instr_ptr));

  z3::expr equality(encoder.encode(write_event, z3));
  z3.solver.add(equality);

  EXPECT_EQ(z3::sat, z3.solver.check());
  
  z3::expr disequality(z3.constant(write_event) != 42);
  z3.solver.add(disequality);
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3ValueEncoderDirectWriteEventThroughDispatch) {
  const unsigned thread_id = 3;

  const Z3ValueEncoder encoder;
  Z3 z3;

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));

  const MemoryAddr write_addr = MemoryAddr::alloc<long>();
  const DirectWriteEvent<long> write_event(thread_id, write_addr, std::move(read_instr_ptr));
  const Event& event = write_event;

  z3::expr equality(event.encode(encoder, z3));
  z3.solver.add(equality);

  EXPECT_EQ(z3::sat, z3.solver.check());
  
  z3::expr disequality(z3.constant(write_event) != 42);
  z3.solver.add(disequality);
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3ValueEncoderIndirectWriteEvent) {
  const unsigned thread_id = 3;
  const size_t array_size = 5;

  const Z3ValueEncoder encoder;
  Z3 z3;

  const MemoryAddr pointer_addr = MemoryAddr::alloc<char[array_size]>();
  std::unique_ptr<ReadEvent<char[array_size]>> pointer_event_ptr(new ReadEvent<char[array_size]>(thread_id, pointer_addr));
  std::unique_ptr<ReadInstr<char[array_size]>> pointer_read_instr(new BasicReadInstr<char[array_size]>(std::move(pointer_event_ptr)));

  std::unique_ptr<ReadInstr<size_t>> offset_read_instr(new LiteralReadInstr<size_t>(7));

  std::unique_ptr<DerefReadInstr<char[array_size], size_t>> deref_read_instr_ptr(
    new DerefReadInstr<char[array_size], size_t>(
      std::move(pointer_read_instr), std::move(offset_read_instr)));

  std::unique_ptr<ReadInstr<char>> read_instr_ptr(new LiteralReadInstr<char>('X'));

  const MemoryAddr write_addr = MemoryAddr::alloc<char>();
  const IndirectWriteEvent<char, size_t, array_size> write_event(thread_id, write_addr,
    std::move(deref_read_instr_ptr), std::move(read_instr_ptr));

  z3.solver.add(encoder.encode(write_event, z3));
  z3::expr new_array(z3.constant(write_event));

  z3.solver.push();

  z3.solver.add(z3::select(new_array, 6) != 'X');
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(z3::select(new_array, 7) != 'X');
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(EncoderTest, Z3ValueEncoderIndirectWriteEventThroughDispatch) {
  const unsigned thread_id = 3;
  const size_t array_size = 5;

  const Z3ValueEncoder encoder;
  Z3 z3;

  const MemoryAddr pointer_addr = MemoryAddr::alloc<char[array_size]>();
  std::unique_ptr<ReadEvent<char[array_size]>> pointer_event_ptr(new ReadEvent<char[array_size]>(thread_id, pointer_addr));
  std::unique_ptr<ReadInstr<char[array_size]>> pointer_read_instr(new BasicReadInstr<char[array_size]>(std::move(pointer_event_ptr)));

  std::unique_ptr<ReadInstr<size_t>> offset_read_instr(new LiteralReadInstr<size_t>(7));

  std::unique_ptr<DerefReadInstr<char[array_size], size_t>> deref_read_instr_ptr(
    new DerefReadInstr<char[array_size], size_t>(
      std::move(pointer_read_instr), std::move(offset_read_instr)));

  std::unique_ptr<ReadInstr<char>> read_instr_ptr(new LiteralReadInstr<char>('X'));

  const MemoryAddr write_addr = MemoryAddr::alloc<char>();
  const IndirectWriteEvent<char, size_t, array_size> write_event(thread_id, write_addr,
    std::move(deref_read_instr_ptr), std::move(read_instr_ptr));

  const Event& event = write_event;

  z3.solver.add(event.encode(encoder, z3));
  z3::expr new_array(z3.constant(write_event));

  z3.solver.push();

  z3.solver.add(z3::select(new_array, 6) != 'X');
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(z3::select(new_array, 7) != 'X');
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(EncoderTest, Z3ValueEncoderReadEvent) {
  const unsigned thread_id = 3;

  const Z3ValueEncoder encoder;
  Z3 z3;

  const MemoryAddr addr = MemoryAddr::alloc<int>();
  const ReadEvent<int> read_event(thread_id, addr);

  z3.solver.add(encoder.encode(read_event, z3));

  // Proves that read events are encoded as false
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3ValueEncoderReadEventThroughDispatch) {
  const unsigned thread_id = 3;

  const Z3ValueEncoder encoder;
  Z3 z3;

  const MemoryAddr addr = MemoryAddr::alloc<int>();
  const ReadEvent<int> read_event(thread_id, addr);
  const Event& event = read_event;

  z3.solver.add(event.encode(encoder, z3));

  // Proves that read events are encoded as a false
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3OrderEncoderForRfWithoutCondition) {
  const unsigned write_thread_id = 7;
  const unsigned read_thread_id = 8;

  const Z3OrderEncoder encoder;
  Z3 z3;

  MemoryAddrRelation<Event> relation;

  const MemoryAddr addr = MemoryAddr::alloc<short>();
  std::unique_ptr<ReadInstr<short>> instr_ptr(new LiteralReadInstr<short>(5));
  const std::shared_ptr<Event> write_event_ptr(
    new DirectWriteEvent<short>(write_thread_id, addr, std::move(instr_ptr)));
  const std::shared_ptr<Event> read_event_ptr(
    new ReadEvent<short>(read_thread_id, addr));

  relation.relate(write_event_ptr);
  relation.relate(read_event_ptr);

  z3.solver.add(encoder.rfe_encode(relation, z3));
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.add(z3.constant(*read_event_ptr) == 3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.add(z3.constant(*write_event_ptr) != 3);
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3OrderEncoderForWsWithoutCondition) {
  const unsigned write_thread_major_id = 7;
  const unsigned write_thread_minor_id = 8;

  const Z3OrderEncoder encoder;
  Z3 z3;

  MemoryAddrRelation<Event> relation;

  const MemoryAddr addr = MemoryAddr::alloc<short>();
  std::unique_ptr<ReadInstr<short>> major_instr_ptr(new LiteralReadInstr<short>(5));
  const std::shared_ptr<Event> major_write_event_ptr(
    new DirectWriteEvent<short>(write_thread_major_id, addr, std::move(major_instr_ptr)));

  std::unique_ptr<ReadInstr<short>> minor_instr_ptr(new LiteralReadInstr<short>(7));
  const std::shared_ptr<Event> minor_write_event_ptr(
    new DirectWriteEvent<short>(write_thread_minor_id, addr, std::move(minor_instr_ptr)));

  relation.relate(major_write_event_ptr);
  relation.relate(minor_write_event_ptr);

  z3.solver.add(encoder.ws_encode(relation, z3));
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.add(z3.clock(*major_write_event_ptr) == z3.clock(*minor_write_event_ptr));
  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(EncoderTest, Z3OrderEncoderForFrWithoutCondition) {
  const unsigned write_thread_id = 7;
  const unsigned read_thread_id = 8;

  const Z3ValueEncoder value_encoder;
  const Z3OrderEncoder order_encoder;
  Z3 z3;

  MemoryAddrRelation<Event> relation;

  const MemoryAddr addr = MemoryAddr::alloc<short>();
  std::unique_ptr<ReadInstr<short>> major_instr_ptr(new LiteralReadInstr<short>(5));
  const std::shared_ptr<Event> major_write_event_ptr(
    new DirectWriteEvent<short>(write_thread_id, addr, std::move(major_instr_ptr)));

  std::unique_ptr<ReadInstr<short>> minor_instr_ptr(new LiteralReadInstr<short>(7));
  const std::shared_ptr<Event> minor_write_event_ptr(
    new DirectWriteEvent<short>(write_thread_id, addr, std::move(minor_instr_ptr)));

  const std::shared_ptr<Event> read_event_ptr(
    new ReadEvent<short>(read_thread_id, addr));

  relation.relate(major_write_event_ptr);
  relation.relate(minor_write_event_ptr);
  relation.relate(read_event_ptr);

  z3.solver.add(major_write_event_ptr->encode(value_encoder, z3));
  z3.solver.add(minor_write_event_ptr->encode(value_encoder, z3));
  z3.solver.add(order_encoder.rfe_encode(relation, z3));
  z3.solver.add(order_encoder.fr_encode(relation, z3));
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.push();

  z3.solver.add(z3.constant(*major_write_event_ptr, *read_event_ptr));
  z3.solver.add(z3.constant(*read_event_ptr) == 5);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(z3.constant(*major_write_event_ptr, *read_event_ptr));
  z3.solver.add(z3.constant(*read_event_ptr) == 7);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(z3.constant(*minor_write_event_ptr, *read_event_ptr));
  z3.solver.add(z3.constant(*read_event_ptr) == 7);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  z3.solver.add(z3.constant(*minor_write_event_ptr, *read_event_ptr));
  z3.solver.add(z3.constant(*read_event_ptr) == 5);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}
