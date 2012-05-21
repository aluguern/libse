#include <sstream>
#include "gtest/gtest.h"
#include "loop.h"
#include "overload.h"

TEST(LoopTest, BoundedUnwindingPolicy) {
  const Value<bool> anything = any_bool("ANY");
  BoundedUnwindingPolicy policy(2);
  EXPECT_TRUE(policy.unwind(anything));
  EXPECT_TRUE(policy.unwind(anything));

  EXPECT_FALSE(policy.unwind(anything));

  EXPECT_FALSE(policy.unwind(anything));
  EXPECT_FALSE(policy.unwind(anything));
  // ...
}

TEST(LoopTest, LoopWithBoundedUnwindingPolicy) {
  const Value<bool> anything = any_bool("ANY");
  std::unique_ptr<UnwindingPolicy> policy_ptr(new BoundedUnwindingPolicy(2));
  Loop loop(std::move(policy_ptr));

  EXPECT_FALSE(policy_ptr);

  EXPECT_TRUE(loop.unwind(anything));
  EXPECT_TRUE(loop.unwind(anything));

  EXPECT_FALSE(loop.unwind(anything));

  EXPECT_FALSE(loop.unwind(anything));
  EXPECT_FALSE(loop.unwind(anything));
  // ...
}

TEST(LoopTest, LoopWithK) {
  const Value<bool> anything = any_bool("ANY");
  Loop loop(2);

  EXPECT_TRUE(loop.unwind(anything));
  EXPECT_TRUE(loop.unwind(anything));

  EXPECT_FALSE(loop.unwind(anything));

  EXPECT_FALSE(loop.unwind(anything));
  EXPECT_FALSE(loop.unwind(anything));
  // ...
}

// Must at least tolerate this API misuse
TEST(LoopTest, NoUnwindButJoin) {
  bool ok;
  Int i = any_int("I");
  Loop loop(2);

  // API misuse because we require that unwind() is called before join().
  std::stringstream out;
  i.get_reflect_value().get_expr()->write(out);
  EXPECT_EQ("[I]", out.str());
}

// Must detect this API misuse
TEST(LoopTest, IgnoredTrueReturnOfUnwind) {
  bool ok;
  Int i = any_int("I");
  Loop loop(2);

  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  // API misuse because we require that begin_loop() and
  // end_loop() are called before join() since the return
  // value of unwind() was true!
  EXPECT_THROW(loop.join(i), LoopError);
}

// Must detect this API misuse
TEST(LoopTest, BeginLoopWithoutUnwind) {
  bool ok;
  Int i = any_int("I");
  Loop loop(2);

  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  loop.begin_loop(i);
  i = i + 1;
  loop.begin_loop(i);

  // API misuse because we require that unwind() is called
  // before every begin_loop() call.
  EXPECT_THROW(loop.join(i), LoopError);
}

// Must detect this API misuse
TEST(LoopTest, BeginLoopWithFalseUnwind) {
  bool ok;
  Int i = any_int("I");
  Loop loop(0);

  ok = loop.unwind(i < 5);
  EXPECT_FALSE(ok);
  loop.begin_loop(i);

  // API misuse because even though unwind() was called before
  // begin_loop() that unwind() call returned false.
  EXPECT_THROW(loop.join(i), LoopError);
}

// Must detect this API misuse
TEST(LoopTest, MultipleUnwindBeforeBeginLoop) {
  bool ok;
  Int i = any_int("I");
  Loop loop(7);

  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  // API misuse because there are more unwind() calls whose
  // return value was true than begin_loop() calls.
  EXPECT_THROW(loop.join(i), LoopError);
}

TEST(LoopTest, Unwind1xWithSingleVar) {
  bool ok;
  Int i = any_int("I");
  Loop loop(2);

  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  loop.begin_loop(i);
  i = i + 1;

  loop.end_loop(i);
  loop.join(i);

  std::stringstream out;
  i.get_reflect_value().get_expr()->write(out);
  EXPECT_EQ("(([I]<5)?([I]+1):[I])", out.str());
}

TEST(LoopTest, Unwind2xWithSingleVar) {
  bool ok;
  Int i = any_int("I");
  Loop loop(7);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  loop.begin_loop(i);
  i = i + 1;

  loop.end_loop(i);

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  loop.begin_loop(i);
  i = i + 2;

  loop.end_loop(i);

  loop.join(i);

  std::stringstream out;
  i.get_reflect_value().get_expr()->write(out);
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?(([I]+1)+2):([I]+1)):[I])", out.str());
}

TEST(LoopTest, Unwind2xWithMultipleVars) {
  bool ok;
  Int i = any_int("I");
  Int j = any_int("J");
  Loop loop(7);

  // 1x
  ok = loop.unwind(i < 5);
  EXPECT_TRUE(ok);

  loop.begin_loop(i);
  loop.begin_loop(j);
  i = i + 1;
  j = j + 1;

  loop.end_loop(i);
  loop.end_loop(j);

  // 2x
  ok = loop.unwind(i < 7);
  EXPECT_TRUE(ok);

  loop.begin_loop(i);
  loop.begin_loop(j);
  i = i + 2;
  j = j + 2;

  loop.end_loop(i);
  loop.end_loop(j);

  loop.join(i);
  loop.join(j);

  std::stringstream i_out;
  i.get_reflect_value().get_expr()->write(i_out);
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?(([I]+1)+2):([I]+1)):[I])", i_out.str());

  std::stringstream j_out;
  j.get_reflect_value().get_expr()->write(j_out);
  EXPECT_EQ("(([I]<5)?((([I]+1)<7)?(([J]+1)+2):([J]+1)):[J])", j_out.str());
}

