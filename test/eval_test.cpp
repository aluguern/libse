#include "gtest/gtest.h"
#include "eval.h"

using namespace se;

TEST(EvalTest, Add) {
  int ret;
  EXPECT_EQ(18, Eval<ADD>::eval(10, 8, ret));
  EXPECT_EQ(18, Eval<ADD>::eval(8, 10, ret));
}

TEST(EvalTest, Land) {
  bool ret;
  EXPECT_TRUE(Eval<LAND>::eval(true, true, ret));
  EXPECT_FALSE(Eval<LAND>::eval(false, true, ret));
  EXPECT_FALSE(Eval<LAND>::eval(true, false, ret));
  EXPECT_FALSE(Eval<LAND>::eval(false, false, ret));
}

TEST(EvalTest, Lor) {
  bool ret;
  EXPECT_TRUE(Eval<LOR>::eval(true, true, ret));
  EXPECT_TRUE(Eval<LOR>::eval(false, true, ret));
  EXPECT_TRUE(Eval<LOR>::eval(true, false, ret));
  EXPECT_FALSE(Eval<LOR>::eval(false, false, ret));
}

TEST(EvalTest, Eql) {
  bool ret;
  EXPECT_TRUE(Eval<EQL>::eval(12, 0xc, ret));
  EXPECT_TRUE(Eval<EQL>::eval(0xc, 12, ret));
  EXPECT_FALSE(Eval<EQL>::eval(12, 13, ret));
}

TEST(EvalTest, Lss) {
  bool ret;
  EXPECT_FALSE(Eval<LSS>::eval(12, 0xc, ret));
  EXPECT_FALSE(Eval<LSS>::eval(0xc, 12, ret));
  EXPECT_TRUE(Eval<LSS>::eval(12, 13, ret));
  EXPECT_FALSE(Eval<LSS>::eval(13, 12, ret));
}

TEST(EvalTest, Not) {
  bool ret;
  EXPECT_FALSE(Eval<NOT>::eval(1, ret));
  EXPECT_FALSE(Eval<NOT>::eval(12, ret));
  EXPECT_TRUE(Eval<NOT>::eval(0, ret));
  EXPECT_TRUE(Eval<NOT>::eval(false, ret));
}
