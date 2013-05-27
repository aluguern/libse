#include "gtest/gtest.h"

#include "concurrent/slicer.h"

using namespace se;

TEST(SlicerTest, ElseDirection) {
  Slicer slicer;

  constexpr Location loc = __COUNTER__;
  constexpr ThreadId thread_id = 3;
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  Threads::reset();
  Threads::begin_main_thread();

  EXPECT_FALSE(slicer.begin_block(loc, condition_ptr));

  const UnaryReadInstr<NOT, bool>& read_instr = dynamic_cast<const UnaryReadInstr<NOT, bool>&>(*ThisThread::path_condition_ptr());
  EXPECT_EQ(condition_ptr.get(), &read_instr.operand_ref());
}

TEST(SlicerTest, ThenDirection) {
  Slicer slicer;

  constexpr Location loc = __COUNTER__;
  constexpr ThreadId thread_id = 3;
  const Zone condition_zone = Zone::unique_atom();

  std::unique_ptr<ReadEvent<bool>> condition_event_ptr(new ReadEvent<bool>(thread_id, condition_zone));
  const std::shared_ptr<ReadInstr<bool>> condition_ptr(
    new BasicReadInstr<bool>(std::move(condition_event_ptr)));

  Threads::reset();
  Threads::begin_main_thread();

  EXPECT_FALSE(slicer.begin_block(loc, condition_ptr));

  EXPECT_TRUE(slicer.next_slice());
  EXPECT_EQ(2, slicer.slice_count());

  Threads::reset();
  Threads::begin_main_thread();

  EXPECT_TRUE(slicer.begin_block(loc, condition_ptr));
  EXPECT_EQ(ThisThread::path_condition_ptr(), condition_ptr);

  EXPECT_FALSE(slicer.next_slice());
  EXPECT_EQ(2, slicer.slice_count());
}
