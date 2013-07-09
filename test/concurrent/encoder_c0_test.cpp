#include <limits>

#include "concurrent/encoder_c0.h"
#include "gtest/gtest.h"

using namespace se;

TEST(EncoderC0Test, Z3BvLiteralBool) {
  Encoders encoders;

  const bool literal = true;
  const LiteralReadInstr<bool> instr(literal);

  encoders.solver.push();

  EXPECT_TRUE(encoders.literal(instr).sort().is_bool());

  encoders.solver.unsafe_add(!encoders.literal(instr));
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(EncoderC0Test, Z3BvLiteral) {
  Encoders encoders;

  const short literal = 3;
  const LiteralReadInstr<short> instr(literal);

#ifdef __USE_BV__
  EXPECT_TRUE(encoders.literal(instr).sort().is_bv());
  EXPECT_EQ(TypeInfo<short>::s_type.bv_size(),
    encoders.literal(instr).sort().bv_size());
#else
  EXPECT_TRUE(encoders.literal(instr).sort().is_int());
#endif

  encoders.solver.push();

  encoders.solver.unsafe_add(encoders.literal(instr) != literal);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  // Sanity check satisfiable formula
  encoders.solver.unsafe_add(encoders.literal(instr) == literal);
  EXPECT_EQ(smt::sat, encoders.solver.check());
}

TEST(EncoderC0Test, Z3BvConstant) {
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  const ReadEvent<int> event(thread_id, zone);

#ifdef __USE_BV__
  EXPECT_TRUE(event.constant(encoders).sort().is_bv());
  EXPECT_EQ(TypeInfo<int>::s_type.bv_size(),
    event.constant(encoders).sort().bv_size());
#else
  EXPECT_TRUE(event.constant(encoders).sort().is_int());
#endif

  encoders.solver.unsafe_add(event.constant(encoders) != event.constant(encoders));

  // Proves that both bit vector constants are equal
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(EncoderC0Test, Z3BoolConstant) {
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  const ReadEvent<bool> event(thread_id, zone);

  EXPECT_TRUE(event.constant(encoders).sort().is_bool());

  encoders.solver.unsafe_add(event.constant(encoders) != event.constant(encoders));

  // Proves that both Boolean constants are equal
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(EncoderC0Test, Z3ArrayConstant) {
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  const ReadEvent<int[5]> read_event(thread_id, zone);

  const smt::UnsafeTerm event_expr(read_event.constant(encoders));
  EXPECT_TRUE(event_expr.sort().is_array());

#ifdef __USE_BV__
  EXPECT_TRUE(event_expr.sort().sorts(0).is_bv());
  EXPECT_EQ(TypeInfo<size_t>::s_type.bv_size(),
    event_expr.sort().sorts(0).bv_size());

  EXPECT_TRUE(event_expr.sort().sorts(1).is_bv());
  EXPECT_EQ(TypeInfo<int>::s_type.bv_size(),
    event_expr.sort().sorts(1).bv_size());
#else
  EXPECT_TRUE(event_expr.sort().sorts(0).is_int());
  EXPECT_TRUE(event_expr.sort().sorts(1).is_int());
#endif

  encoders.solver.push();

  encoders.solver.unsafe_add(event_expr != read_event.constant(encoders));

  // Proves that both bit vector constants are equal
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  // Sanity checks ...

  // 1:
  encoders.solver.push();

  encoders.solver.unsafe_add(event_expr == read_event.constant(encoders));
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  // 2:
  encoders.solver.push();

#ifdef __USE_BV__
  const smt::UnsafeTerm index = smt::literal<smt::Bv<size_t>>(3);
  const smt::UnsafeTerm value_a = smt::literal<smt::Bv<int>>(7);
  const smt::UnsafeTerm value_b = smt::literal<smt::Bv<int>>(8);
#else
  const smt::UnsafeTerm index = smt::literal<smt::Int>(3);
  const smt::UnsafeTerm value_a = smt::literal<smt::Int>(7);
  const smt::UnsafeTerm value_b = smt::literal<smt::Int>(8);
#endif
  smt::UnsafeTerm array_a(smt::store(event_expr, index, value_a));
  smt::UnsafeTerm array_b(smt::store(read_event.constant(encoders), index, value_b));

  encoders.solver.unsafe_add(array_a == array_b);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  // 3:
  encoders.solver.push();

  encoders.solver.unsafe_add(array_a != array_b);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  // 4:
  encoders.solver.push();

  encoders.solver.unsafe_add(read_event.constant(encoders) == array_b);
  encoders.solver.unsafe_add(smt::select(event_expr, index) == value_b);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  // 5:
  encoders.solver.push();

  encoders.solver.unsafe_add(read_event.constant(encoders) == array_b);
  encoders.solver.unsafe_add(smt::select(event_expr, index) != value_b);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(EncoderC0Test, Z3IndirectWriteEventConstant) {
  const unsigned thread_id = 3;
  const size_t array_size = 5;

  Encoders encoders;

  const Zone pointer_zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<char[array_size]>> pointer_event_ptr(new ReadEvent<char[array_size]>(thread_id, pointer_zone));
  std::unique_ptr<ReadInstr<char[array_size]>> pointer_read_instr(new BasicReadInstr<char[array_size]>(std::move(pointer_event_ptr)));

  std::unique_ptr<ReadInstr<size_t>> offset_read_instr(new LiteralReadInstr<size_t>(7));

  std::unique_ptr<DerefReadInstr<char[array_size], size_t>> deref_read_instr_ptr(
    new DerefReadInstr<char[array_size], size_t>(
      std::move(pointer_read_instr), std::move(offset_read_instr)));

  std::unique_ptr<ReadInstr<char>> read_instr_ptr(new LiteralReadInstr<char>('X'));

  const Zone write_zone = Zone::unique_atom();
  const IndirectWriteEvent<char, size_t, array_size> write_event(thread_id, write_zone,
    std::move(deref_read_instr_ptr), std::move(read_instr_ptr));

  const smt::UnsafeTerm event_expr(write_event.constant(encoders));
  EXPECT_TRUE(event_expr.sort().is_array());

#ifdef __USE_BV__
  EXPECT_TRUE(event_expr.sort().sorts(0).is_bv());
  EXPECT_EQ(TypeInfo<size_t>::s_type.bv_size(),
    event_expr.sort().sorts(0).bv_size());

  EXPECT_TRUE(event_expr.sort().sorts(1).is_bv());
  EXPECT_EQ(TypeInfo<char>::s_type.bv_size(),
    event_expr.sort().sorts(1).bv_size());
#else
  EXPECT_TRUE(event_expr.sort().sorts(0).is_int());
  EXPECT_TRUE(event_expr.sort().sorts(1).is_int());
#endif
}

TEST(EncoderC0Test, Z3ReadClock) {
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  const ReadEvent<int> event(thread_id, zone);

  encoders.solver.push();
#ifdef __USE_MATRIX__
  encoders.solver.unsafe_add(encoders.clock(event).simultaneous_or_happens_before(Clock("epoch"));
#else
  encoders.solver.unsafe_add(encoders.clock(event).term() <= 0);
#endif

  // Proves that clock values are natural numbers
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  // Sanity check a satisfiable formula
  encoders.solver.pop();
#ifndef __USE_MATRIX__
  encoders.solver.unsafe_add(encoders.clock(event).term() <= 1);
  EXPECT_EQ(smt::sat, encoders.solver.check());
#endif
}

TEST(EncoderC0Test, Z3WriteClock) {
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  const DirectWriteEvent<int> event(thread_id, zone,
    std::unique_ptr<ReadInstr<int>>(new LiteralReadInstr<int>(42)));

  encoders.solver.push();
#ifdef __USE_MATRIX__
  encoders.solver.unsafe_add(encoders.clock(event).simultaneous_or_happens_before(Clock("epoch"));
#else
  encoders.solver.unsafe_add(encoders.clock(event).term() <= 0);
#endif

  // Proves that clock values are natural numbers
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  // Sanity check a satisfiable formula
  encoders.solver.pop();
#ifndef __USE_MATRIX__
  encoders.solver.unsafe_add(encoders.clock(event).term() <= 1);
  EXPECT_EQ(smt::sat, encoders.solver.check());
#endif
}

TEST(EncoderC0Test, ReadInstrEncoderForLiteralReadInstr) {
  const ReadInstrEncoder encoder;
  Encoders encoders;

  const short literal = 3;
  const LiteralReadInstr<short> instr(literal);

  encoders.solver.push();

  encoders.solver.unsafe_add(encoder.encode(instr, encoders) != literal);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  encoders.solver.unsafe_add(encoder.encode(instr, encoders) != encoders.literal(instr));
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  // Sanity check satisfiable formula
  encoders.solver.unsafe_add(encoder.encode(instr, encoders) == literal);
  EXPECT_EQ(smt::sat, encoders.solver.check());
}

TEST(EncoderC0Test, ReadInstrEncoderForBasicReadInstrAsBv) {
  const ReadInstrEncoder encoder;
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, zone));
  const BasicReadInstr<int> instr(std::move(event_ptr));

  encoders.solver.unsafe_add(encoder.encode(instr, encoders) != instr.event_ptr()->constant(encoders));

  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(EncoderC0Test, ReadInstrEncoderForBasicReadInstrAsArray) {
  const ReadInstrEncoder encoder;
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<char[5]>> event_ptr(new ReadEvent<char[5]>(thread_id, zone));
  const BasicReadInstr<char[5]> instr(std::move(event_ptr));

  // See also Z3ArrayConstant test
  encoders.solver.unsafe_add(encoder.encode(instr, encoders) != instr.event_ptr()->constant(encoders));
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  const smt::UnsafeTerm& expr = encoder.encode(instr, encoders);

  EXPECT_TRUE(expr.sort().is_array());

#ifdef __USE_BV__
  EXPECT_TRUE(expr.sort().sorts(0).is_bv());
  EXPECT_EQ(TypeInfo<size_t>::s_type.bv_size(),
    expr.sort().sorts(0).bv_size());

  EXPECT_TRUE(expr.sort().sorts(1).is_bv());
  EXPECT_EQ(TypeInfo<char>::s_type.bv_size(),
    expr.sort().sorts(1).bv_size());
#else
  EXPECT_TRUE(expr.sort().sorts(0).is_int());
  EXPECT_TRUE(expr.sort().sorts(1).is_int());
#endif
}

TEST(EncoderC0Test, ReadInstrEncoderForUnaryReadInstrAsLiteralBool) {
  const ReadInstrEncoder encoder;
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<int>> event_ptr(new ReadEvent<int>(thread_id, zone));
  std::unique_ptr<ReadInstr<bool>> instr_ptr(new LiteralReadInstr<bool>(false));
  const UnaryReadInstr<NOT, bool> instr(std::move(instr_ptr));

  EXPECT_TRUE(encoder.encode(instr, encoders).sort().is_bool());
  encoders.solver.unsafe_add(!encoder.encode(instr, encoders));

  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(EncoderC0Test, ReadInstrEncoderForUnaryReadInstrAsLiteralInteger) {
  const ReadInstrEncoder encoder;
  Encoders encoders;

  std::unique_ptr<ReadInstr<short>> instr_ptr(new LiteralReadInstr<short>(7));
  const UnaryReadInstr<SUB, short> instr(std::move(instr_ptr));

#ifdef __USE_BV__
  EXPECT_TRUE(encoder.encode(instr, encoders).sort().is_bv());
#else
  EXPECT_TRUE(encoder.encode(instr, encoders).sort().is_int());
#endif
  encoders.solver.unsafe_add(encoder.encode(instr, encoders) != -7);

  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

#ifdef __USE_BV__
TEST(EncoderC0Test, ReadInstrEncoderForUnaryReadInstrAsInteger) {
  const ReadInstrEncoder encoder;
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<short>> event_ptr(new ReadEvent<short>(thread_id, zone));
  std::unique_ptr<ReadInstr<short>> instr_ptr(new BasicReadInstr<short>(std::move(event_ptr)));
  const UnaryReadInstr<SUB, short> instr(std::move(instr_ptr));

  EXPECT_TRUE(encoder.encode(instr, encoders).sort().is_bv());

  encoders.solver.push();

  encoders.solver.unsafe_add(encoder.encode(instr, encoders) != -instr.operand_ref().encode(encoder, encoders));
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.unsafe_add(instr.operand_ref().encode(encoder, encoders) < 0);
  encoders.solver.unsafe_add(encoder.encode(instr, encoders) < 0);

  // Finds an overflow to satisfy the formulas
  EXPECT_EQ(smt::sat, encoders.solver.check());

  // Since we prevent overflows, the assertions are now unsatisfiable.
  encoders.solver.unsafe_add(instr.operand_ref().encode(encoder, encoders) > std::numeric_limits<short>::min());
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}
#endif

TEST(EncoderC0Test, ReadInstrEncoderForBinaryReadInstrAsBool) {
  const ReadInstrEncoder encoder;
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<short>> levent_ptr(new ReadEvent<short>(thread_id, zone));
  std::unique_ptr<ReadInstr<short>> linstr_ptr(new BasicReadInstr<short>(std::move(levent_ptr)));

  std::unique_ptr<ReadInstr<short>> rinstr_ptr(new LiteralReadInstr<short>(7));

  const BinaryReadInstr<LSS, short, short> instr(std::move(linstr_ptr), std::move(rinstr_ptr));

  EXPECT_TRUE(encoder.encode(instr, encoders).sort().is_bool());
  encoders.solver.unsafe_add(encoder.encode(instr, encoders));

  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.unsafe_add(instr.loperand_ref().encode(encoder, encoders) >= 7);

  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

#ifdef __USE_BV__
TEST(EncoderC0Test, ReadInstrEncoderForBinaryReadInstrAsInteger) {
  const ReadInstrEncoder encoder;
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<short>> levent_ptr(new ReadEvent<short>(thread_id, zone));
  std::unique_ptr<ReadInstr<short>> linstr_ptr(new BasicReadInstr<short>(std::move(levent_ptr)));

  std::unique_ptr<ReadInstr<short>> rinstr_ptr(new LiteralReadInstr<short>(7));

  const BinaryReadInstr<ADD, short, short> instr(std::move(linstr_ptr), std::move(rinstr_ptr));

  EXPECT_TRUE(encoder.encode(instr, encoders).sort().is_bv());
  encoders.solver.unsafe_add(encoder.encode(instr, encoders) <= instr.loperand_ref().encode(encoder, encoders));

  // Finds an overflow to satisfy the formulas
  EXPECT_EQ(smt::sat, encoders.solver.check());

  const short upper_bound = std::numeric_limits<short>::max() - 6;
  smt::UnsafeTerm v_upper_bound = smt::literal<smt::Bv<short>>(upper_bound);
  encoders.solver.unsafe_add(instr.loperand_ref().encode(encoder, encoders) < v_upper_bound);

  // Since we prevent overflows, the assertions are now unsatisfiable.
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}
#endif

TEST(EncoderC0Test, ReadInstrEncoderForDerefReadInstrAsInteger) {
  const ReadInstrEncoder encoder;
  Encoders encoders;

  const unsigned thread_id = 3;
  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<short[5]>> event_ptr(new ReadEvent<short[5]>(thread_id, zone));
  std::unique_ptr<ReadInstr<short[5]>> memory_ptr(new BasicReadInstr<short[5]>(std::move(event_ptr)));

  std::unique_ptr<ReadInstr<size_t>> offset_ptr(
    new LiteralReadInstr<size_t>(/* out of bounds allowed for now */ 7));

  const DerefReadInstr<short[5], size_t> instr(std::move(memory_ptr), std::move(offset_ptr));

  // See also ReadInstrEncoderForBasicReadInstrAsArray test
  const BasicReadInstr<short[5]>& array_read_instr =
    static_cast<const BasicReadInstr<short[5]>&>(instr.memory_ref());
  EXPECT_TRUE(array_read_instr.encode(encoder, encoders).sort().is_array());
}

TEST(EncoderC0Test, ValueEncoderDirectWriteEvent) {
  const unsigned thread_id = 3;

  const ValueEncoder encoder;
  Encoders encoders;

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));

  const Zone write_zone = Zone::unique_atom();
  const DirectWriteEvent<long> write_event(thread_id, write_zone, std::move(read_instr_ptr));

  smt::UnsafeTerm equality(encoder.encode_eq(write_event, encoders));
  encoders.solver.unsafe_add(equality);

  EXPECT_EQ(smt::sat, encoders.solver.check());

#ifdef __USE_BV__
  smt::UnsafeTerm literal = smt::literal<smt::Bv<long>>(42);
#else
  smt::UnsafeTerm literal = smt::literal<smt::Int>(42);
#endif
  smt::UnsafeTerm disequality(write_event.constant(encoders) != literal);
  encoders.solver.unsafe_add(disequality);
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(EncoderC0Test, ValueEncoderDirectWriteEventThroughDispatch) {
  const unsigned thread_id = 3;

  const ValueEncoder encoder;
  Encoders encoders;

  std::unique_ptr<ReadInstr<long>> read_instr_ptr(new LiteralReadInstr<long>(42L));

  const Zone write_zone = Zone::unique_atom();
  const DirectWriteEvent<long> write_event(thread_id, write_zone, std::move(read_instr_ptr));
  const Event& event = write_event;

  smt::UnsafeTerm equality(event.encode_eq(encoder, encoders));
  encoders.solver.unsafe_add(equality);

  EXPECT_EQ(smt::sat, encoders.solver.check());
  
#ifdef __USE_BV__
  smt::UnsafeTerm literal = smt::literal<smt::Bv<long>>(42);
#else
  smt::UnsafeTerm literal = smt::literal<smt::Int>(42);
#endif
  smt::UnsafeTerm disequality(write_event.constant(encoders) != literal);
  encoders.solver.unsafe_add(disequality);
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(EncoderC0Test, ValueEncoderIndirectWriteEvent) {
  const unsigned thread_id = 3;
  const size_t array_size = 5;

  const ValueEncoder encoder;
  Encoders encoders;

  const Zone pointer_zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<char[array_size]>> pointer_event_ptr(new ReadEvent<char[array_size]>(thread_id, pointer_zone));
  std::unique_ptr<ReadInstr<char[array_size]>> pointer_read_instr(new BasicReadInstr<char[array_size]>(std::move(pointer_event_ptr)));

  std::unique_ptr<ReadInstr<size_t>> offset_read_instr(new LiteralReadInstr<size_t>(7));

  std::unique_ptr<DerefReadInstr<char[array_size], size_t>> deref_read_instr_ptr(
    new DerefReadInstr<char[array_size], size_t>(
      std::move(pointer_read_instr), std::move(offset_read_instr)));

  std::unique_ptr<ReadInstr<char>> read_instr_ptr(new LiteralReadInstr<char>('X'));

  const Zone write_zone = Zone::unique_atom();
  const IndirectWriteEvent<char, size_t, array_size> write_event(thread_id, write_zone,
    std::move(deref_read_instr_ptr), std::move(read_instr_ptr));

  encoders.solver.unsafe_add(encoder.encode_eq(write_event, encoders));
  smt::UnsafeTerm new_array(write_event.constant(encoders));

  encoders.solver.push();

#ifdef __USE_BV__
  smt::UnsafeTerm literal = smt::literal<smt::Bv<char>>('X');
  smt::UnsafeTerm index_6 = smt::literal<smt::Bv<size_t>>(6);
#else
  smt::UnsafeTerm literal = smt::literal<smt::Int>('X');
  smt::UnsafeTerm index_6 = smt::literal<smt::Int>(6);
#endif
  encoders.solver.unsafe_add(smt::select(new_array, index_6) != literal);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

#ifdef __USE_BV__
  smt::UnsafeTerm index_7 = smt::literal<smt::Bv<size_t>>(7);
#else
  smt::UnsafeTerm index_7 = smt::literal<smt::Int>(7);
#endif
  encoders.solver.unsafe_add(smt::select(new_array, index_7) != literal);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(EncoderC0Test, ValueEncoderIndirectWriteEventThroughDispatch) {
  const unsigned thread_id = 3;
  const size_t array_size = 5;

  const ValueEncoder encoder;
  Encoders encoders;

  const Zone pointer_zone = Zone::unique_atom();
  std::unique_ptr<ReadEvent<char[array_size]>> pointer_event_ptr(new ReadEvent<char[array_size]>(thread_id, pointer_zone));
  std::unique_ptr<ReadInstr<char[array_size]>> pointer_read_instr(new BasicReadInstr<char[array_size]>(std::move(pointer_event_ptr)));

  std::unique_ptr<ReadInstr<size_t>> offset_read_instr(new LiteralReadInstr<size_t>(7));

  std::unique_ptr<DerefReadInstr<char[array_size], size_t>> deref_read_instr_ptr(
    new DerefReadInstr<char[array_size], size_t>(
      std::move(pointer_read_instr), std::move(offset_read_instr)));

  std::unique_ptr<ReadInstr<char>> read_instr_ptr(new LiteralReadInstr<char>('X'));

  const Zone write_zone = Zone::unique_atom();
  const IndirectWriteEvent<char, size_t, array_size> write_event(thread_id, write_zone,
    std::move(deref_read_instr_ptr), std::move(read_instr_ptr));

  const Event& event = write_event;

  encoders.solver.unsafe_add(event.encode_eq(encoder, encoders));
  smt::UnsafeTerm new_array(write_event.constant(encoders));

  encoders.solver.push();

#ifdef __USE_BV__
  smt::UnsafeTerm literal = smt::literal<smt::Bv<char>>('X');
  smt::UnsafeTerm index_6 = smt::literal<smt::Bv<size_t>>(6);
#else
  smt::UnsafeTerm literal = smt::literal<smt::Int>('X');
  smt::UnsafeTerm index_6 = smt::literal<smt::Int>(6);
#endif
  encoders.solver.unsafe_add(smt::select(new_array, index_6) != literal);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

#ifdef __USE_BV__
  smt::UnsafeTerm index_7 = smt::literal<smt::Bv<size_t>>(7);
#else
  smt::UnsafeTerm index_7 = smt::literal<smt::Int>(7);
#endif
  encoders.solver.unsafe_add(smt::select(new_array, index_7) != literal);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(EncoderC0Test, ValueEncoderReadEvent) {
  const unsigned thread_id = 3;

  const ValueEncoder encoder;
  Encoders encoders;

  const Zone zone = Zone::unique_atom();
  const ReadEvent<int> read_event(thread_id, zone);

  encoders.solver.unsafe_add(encoder.encode_eq(read_event, encoders));

  // Proves that read events are encoded as false
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(EncoderC0Test, ValueEncoderReadEventThroughDispatch) {
  const unsigned thread_id = 3;

  const ValueEncoder encoder;
  Encoders encoders;

  const Zone zone = Zone::unique_atom();
  const ReadEvent<int> read_event(thread_id, zone);
  const Event& event = read_event;

  encoders.solver.unsafe_add(event.encode_eq(encoder, encoders));

  // Proves that read events are encoded as a false
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(EncoderC0Test, Z3OrderEncoderC0ForRfWithoutCondition) {
  const unsigned write_thread_id = 7;
  const unsigned read_thread_id = 8;

  const Z3OrderEncoderC0 encoder;
  Encoders encoders;

  ZoneRelation<Event> relation;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadInstr<short>> instr_ptr(new LiteralReadInstr<short>(5));
  const std::shared_ptr<Event> write_event_ptr(
    new DirectWriteEvent<short>(write_thread_id, zone, std::move(instr_ptr)));
  const std::shared_ptr<Event> read_event_ptr(
    new ReadEvent<short>(read_thread_id, zone));

  relation.relate(write_event_ptr);
  relation.relate(read_event_ptr);

  encoders.solver.unsafe_add(encoder.rf_enc(relation, encoders));
  EXPECT_EQ(smt::sat, encoders.solver.check());

#ifdef __USE_BV__
  smt::UnsafeTerm v_3 = smt::literal<smt::Bv<short>>(3);
#else
  smt::UnsafeTerm v_3 = smt::literal<smt::Int>(3);
#endif
  encoders.solver.unsafe_add(read_event_ptr->constant(encoders) == v_3);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.unsafe_add(write_event_ptr->constant(encoders) != v_3);
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(EncoderC0Test, Z3OrderEncoderC0ForWsWithoutCondition) {
  const unsigned write_thread_major_id = 7;
  const unsigned write_thread_minor_id = 8;

  const Z3OrderEncoderC0 encoder;
  Encoders encoders;

  ZoneRelation<Event> relation;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadInstr<short>> major_instr_ptr(new LiteralReadInstr<short>(5));
  const std::shared_ptr<Event> major_write_event_ptr(
    new DirectWriteEvent<short>(write_thread_major_id, zone, std::move(major_instr_ptr)));

  std::unique_ptr<ReadInstr<short>> minor_instr_ptr(new LiteralReadInstr<short>(7));
  const std::shared_ptr<Event> minor_write_event_ptr(
    new DirectWriteEvent<short>(write_thread_minor_id, zone, std::move(minor_instr_ptr)));

  relation.relate(major_write_event_ptr);
  relation.relate(minor_write_event_ptr);

  encoders.solver.unsafe_add(encoder.ws_enc(relation, encoders));
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.unsafe_add(encoders.clock(*major_write_event_ptr).simultaneous(encoders.clock(*minor_write_event_ptr)));
  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(EncoderC0Test, Z3OrderEncoderC0ForFrWithoutCondition) {
  const unsigned write_thread_id = 7;
  const unsigned read_thread_id = 8;

  const ValueEncoder value_encoder;
  const Z3OrderEncoderC0 order_encoder;
  Encoders encoders;

  ZoneRelation<Event> relation;

  const Zone zone = Zone::unique_atom();
  std::unique_ptr<ReadInstr<short>> major_instr_ptr(new LiteralReadInstr<short>(5));
  const std::shared_ptr<Event> major_write_event_ptr(
    new DirectWriteEvent<short>(write_thread_id, zone, std::move(major_instr_ptr)));

  std::unique_ptr<ReadInstr<short>> minor_instr_ptr(new LiteralReadInstr<short>(7));
  const std::shared_ptr<Event> minor_write_event_ptr(
    new DirectWriteEvent<short>(write_thread_id, zone, std::move(minor_instr_ptr)));

  const std::shared_ptr<Event> read_event_ptr(
    new ReadEvent<short>(read_thread_id, zone));

  relation.relate(major_write_event_ptr);
  relation.relate(minor_write_event_ptr);
  relation.relate(read_event_ptr);

  encoders.solver.unsafe_add(major_write_event_ptr->encode_eq(value_encoder, encoders));
  encoders.solver.unsafe_add(minor_write_event_ptr->encode_eq(value_encoder, encoders));

  order_encoder.encode_without_ws(relation, encoders);

  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.push();

  encoders.solver.unsafe_add(encoders.rf(*major_write_event_ptr, *read_event_ptr));
#ifdef __USE_BV__
  smt::UnsafeTerm v_5 = smt::literal<smt::Bv<short>>(5);
#else
  smt::UnsafeTerm v_5 = smt::literal<smt::Int>(5);
#endif
  encoders.solver.unsafe_add(read_event_ptr->constant(encoders) == v_5);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  encoders.solver.unsafe_add(encoders.rf(*major_write_event_ptr, *read_event_ptr));
#ifdef __USE_BV__
  smt::UnsafeTerm v_7 = smt::literal<smt::Bv<short>>(7);
#else
  smt::UnsafeTerm v_7 = smt::literal<smt::Int>(7);
#endif
  encoders.solver.unsafe_add(read_event_ptr->constant(encoders) == v_7);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  encoders.solver.unsafe_add(encoders.rf(*minor_write_event_ptr, *read_event_ptr));
  encoders.solver.unsafe_add(read_event_ptr->constant(encoders) == v_7);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  encoders.solver.unsafe_add(encoders.rf(*minor_write_event_ptr, *read_event_ptr));
  encoders.solver.unsafe_add(read_event_ptr->constant(encoders) == v_5);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}
