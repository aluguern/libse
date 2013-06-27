#include "concurrent/slicer.h"
#include "gtest/gtest.h"

using namespace se;

TEST(SlicerTest, ZeroSliceFreq) {
  Slicer slicer;

  constexpr Location loc = __COUNTER__;
  constexpr ThreadId thread_id = 3;
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  Threads::reset();
  Threads::begin_main_thread();

  EXPECT_TRUE(slicer.begin_then_branch(loc, condition_ptr));
  EXPECT_EQ(condition_ptr, ThisThread::path_condition_ptr());

  EXPECT_TRUE(slicer.begin_else_branch(loc + 1));

  const UnaryReadInstr<NOT, bool>& read_instr = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*ThisThread::path_condition_ptr());
  EXPECT_EQ(condition_ptr.get(), &read_instr.operand_ref());

  EXPECT_FALSE(slicer.next_slice());
}

TEST(SlicerTest, MaxSliceFreqElse) {
  Slicer slicer(MAX_SLICE_FREQ);

  constexpr Location loc = __COUNTER__;
  constexpr ThreadId thread_id = 3;
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  Threads::reset();
  Threads::begin_main_thread();

  EXPECT_FALSE(slicer.begin_then_branch(loc, condition_ptr));
  EXPECT_TRUE(slicer.begin_else_branch(loc + 1));

  const UnaryReadInstr<NOT, bool>& read_instr = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*ThisThread::path_condition_ptr());
  EXPECT_EQ(condition_ptr.get(), &read_instr.operand_ref());
}

TEST(SlicerTest, MaxSliceFreqThen) {
  Slicer slicer(MAX_SLICE_FREQ);

  constexpr Location loc = __COUNTER__;
  constexpr ThreadId thread_id = 3;
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  Threads::reset();
  Threads::begin_main_thread();

  EXPECT_FALSE(slicer.begin_then_branch(loc, condition_ptr));

  EXPECT_TRUE(slicer.next_slice());
  EXPECT_EQ(2, slicer.slice_count());

  Threads::reset();
  Threads::begin_main_thread();

  EXPECT_TRUE(slicer.begin_then_branch(loc, condition_ptr));
  EXPECT_EQ(condition_ptr, ThisThread::path_condition_ptr());

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(2, slicer.slice_count());
}
