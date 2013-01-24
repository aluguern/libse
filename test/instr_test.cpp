#include "gtest/gtest.h"
#include "instr.h"

using namespace se;

TEST(InstrTest, BinaryBothSymbolic) {
  Value<int> larg = any<int>("A");
  Value<char> rarg = any<char>("B");
  Value<int> ret = any<char>("C");

  Instr<ADD>::exec(larg, rarg, ret);

  std::stringstream out;
  out << ret.expr();
  EXPECT_EQ("([A]+[B])", out.str());
}

TEST(InstrTest, BinaryLeftSymbolicAndRightConcrete) {
  Value<int> larg = any<int>("A");
  Value<char> rarg = make_value('h');
  Value<int> ret = any<char>("C");

  Instr<ADD>::exec(larg, rarg, ret);

  std::stringstream out;
  out << ret.expr();
  EXPECT_EQ("([A]+h)", out.str());
}

TEST(InstrTest, BinaryLeftSymbolicAndRightBasic) {
  Value<int> larg = any<int>("A");
  int rarg = 12;
  Value<int> ret = any<char>("C");

  Instr<ADD>::exec(larg, rarg, ret);

  std::stringstream out;
  out << ret.expr();
  EXPECT_EQ("([A]+12)", out.str());
}

TEST(InstrTest, BinaryLeftConcreteAndRightSymbolic) {
  Value<char> larg = make_value('h');
  Value<int> rarg = any<int>("A");
  Value<int> ret = any<char>("C");

  Instr<ADD>::exec(larg, rarg, ret);

  std::stringstream out;
  out << ret.expr();
  EXPECT_EQ("(h+[A])", out.str());
}

TEST(InstrTest, BinaryLeftBasicAndRightSymbolic) {
  int larg = 12;
  Value<int> rarg = any<int>("A");
  Value<int> ret = any<char>("C");

  Instr<ADD>::exec(larg, rarg, ret);

  std::stringstream out;
  out << ret.expr();
  EXPECT_EQ("(12+[A])", out.str());
}

TEST(InstrTest, BinaryAllConcrete) {
  Value<char> larg = make_value('h');
  Value<int> rarg = make_value(12);
  Value<int> ret = any<int>("C");

  Instr<ADD>::exec(larg, rarg, ret);

  std::stringstream out;
  out << ret.expr();
  EXPECT_EQ("[C]", out.str());
}

TEST(InstrTest, UnarySymbolic) {
  Value<int> arg = any<int>("A");
  Value<int> ret = any<int>("C");

  Instr<NOT>::exec(arg, ret);

  std::stringstream out;
  out << ret.expr();
  EXPECT_EQ("(![A])", out.str());
}

TEST(InstrTest, UnaryConcrete) {
  Value<int> arg = make_value(12);
  Value<int> ret = any<int>("C");

  Instr<NOT>::exec(arg, ret);

  std::stringstream out;
  out << ret.expr();
  EXPECT_EQ("[C]", out.str());
}
