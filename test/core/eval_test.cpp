#include "gtest/gtest.h"
#include "core/eval.h"

using namespace se;

TEST(EvalTest, Not) {
  EXPECT_FALSE(Eval<NOT>::eval(1));
  EXPECT_FALSE(Eval<NOT>::eval(12));
  EXPECT_TRUE(Eval<NOT>::eval(0));
  EXPECT_TRUE(Eval<NOT>::eval(false));
}

TEST(EvalTest, Add) {
  EXPECT_EQ(18, Eval<ADD>::eval(10, 8));
  EXPECT_EQ(18, Eval<ADD>::eval(8, 10));
}

TEST(EvalTest, Land) {
  EXPECT_TRUE(Eval<LAND>::eval(true, true));
  EXPECT_FALSE(Eval<LAND>::eval(false, true));
  EXPECT_FALSE(Eval<LAND>::eval(true, false));
  EXPECT_FALSE(Eval<LAND>::eval(false, false));
}

TEST(EvalTest, Lor) {
  EXPECT_TRUE(Eval<LOR>::eval(true, true));
  EXPECT_TRUE(Eval<LOR>::eval(false, true));
  EXPECT_TRUE(Eval<LOR>::eval(true, false));
  EXPECT_FALSE(Eval<LOR>::eval(false, false));
}

TEST(EvalTest, Eql) {
  EXPECT_TRUE(Eval<EQL>::eval(12, 0xc));
  EXPECT_TRUE(Eval<EQL>::eval(0xc, 12));
  EXPECT_FALSE(Eval<EQL>::eval(12, 13));
}

TEST(EvalTest, Lss) {
  EXPECT_FALSE(Eval<LSS>::eval(12, 0xc));
  EXPECT_FALSE(Eval<LSS>::eval(0xc, 12));
  EXPECT_TRUE(Eval<LSS>::eval(12, 13));
  EXPECT_FALSE(Eval<LSS>::eval(13, 12));
}

TEST(EvalTest, ConstNot) {
  EXPECT_FALSE(Eval<NOT>::const_eval(1));
  EXPECT_FALSE(Eval<NOT>::const_eval(12));
  EXPECT_TRUE(Eval<NOT>::const_eval(0));
  EXPECT_TRUE(Eval<NOT>::const_eval(false));
}

TEST(EvalTest, ConstAdd) {
  EXPECT_EQ(18, Eval<ADD>::const_eval(10, 8));
  EXPECT_EQ(18, Eval<ADD>::const_eval(8, 10));
}

TEST(EvalTest, ConstLand) {
  EXPECT_TRUE(Eval<LAND>::const_eval(true, true));
  EXPECT_FALSE(Eval<LAND>::const_eval(false, true));
  EXPECT_FALSE(Eval<LAND>::const_eval(true, false));
  EXPECT_FALSE(Eval<LAND>::const_eval(false, false));
}

TEST(EvalTest, ConstLor) {
  EXPECT_TRUE(Eval<LOR>::const_eval(true, true));
  EXPECT_TRUE(Eval<LOR>::const_eval(false, true));
  EXPECT_TRUE(Eval<LOR>::const_eval(true, false));
  EXPECT_FALSE(Eval<LOR>::const_eval(false, false));
}

TEST(EvalTest, ConstEql) {
  EXPECT_TRUE(Eval<EQL>::const_eval(12, 0xc));
  EXPECT_TRUE(Eval<EQL>::const_eval(0xc, 12));
  EXPECT_FALSE(Eval<EQL>::const_eval(12, 13));
}

TEST(EvalTest, ConstLss) {
  EXPECT_FALSE(Eval<LSS>::const_eval(12, 0xc));
  EXPECT_FALSE(Eval<LSS>::const_eval(0xc, 12));
  EXPECT_TRUE(Eval<LSS>::const_eval(12, 13));
  EXPECT_FALSE(Eval<LSS>::const_eval(13, 12));
}
