#include "gtest/gtest.h"

#include "concurrent/path_condition.h"

using namespace se;

TEST(PathConditionTest, EmptyPathCondition) {
  PathCondition path_condition;
  EXPECT_EQ(nullptr, path_condition.top());
}

TEST(PathConditionTest, NonemptyPathCondition) {
  std::unique_ptr<ReadInstr<bool>> true_condition(
    new LiteralReadInstr<bool>(true, nullptr));

  std::unique_ptr<ReadInstr<bool>> false_condition(
    new LiteralReadInstr<bool>(false, nullptr));

  PathCondition path_condition;
  path_condition.push(std::move(true_condition));
  path_condition.push(std::move(false_condition));

  const LiteralReadInstr<bool>& false_read_instr =
    dynamic_cast<const LiteralReadInstr<bool>&>(*path_condition.top());
  EXPECT_FALSE(false_read_instr.literal());

  path_condition.pop();

  const LiteralReadInstr<bool>& true_read_instr =
    dynamic_cast<const LiteralReadInstr<bool>&>(*path_condition.top());
  EXPECT_TRUE(true_read_instr.literal());

  path_condition.pop();

  EXPECT_EQ(nullptr, path_condition.top());
}
