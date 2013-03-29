#include "gtest/gtest.h"

#include "concurrent/thread.h"

using namespace se;

bool function_call;

void f1() { function_call = true; }

void f2(short x, char y) {
  if (x == 5 && y == 'Z') { function_call = true; }
}

void f3(bool* function_call_ptr) {
  *function_call_ptr = true;
}

TEST(ThreadTest, FunctionCallWithoutArgs) {
  function_call = false;

  EXPECT_FALSE(function_call);
  Thread thread(f1);
  EXPECT_TRUE(function_call);
}

TEST(ThreadTest, FunctionCallWithArgs) {
  function_call = false;
  char y = 'Z';

  EXPECT_FALSE(function_call);
  Thread thread(f2, 5, y);
  EXPECT_TRUE(function_call);
}

TEST(ThreadTest, FunctionCallWithPointerArgs) {
  function_call = false;
  bool* function_call_ptr = &function_call;

  EXPECT_FALSE(function_call);
  Thread thread(f3, function_call_ptr);
  EXPECT_TRUE(function_call);
}
