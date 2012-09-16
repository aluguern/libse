#include <sstream>
#include "gtest/gtest.h"
#include "sequential-se.h"

using namespace se;

TEST(VersionTest, InitWithConcrete) {
  Bool a = true;
  Bool b = true;
  Char c = 'a';
  Int d = 3;

  EXPECT_EQ(0, a.version());
  EXPECT_EQ(0, b.version());
  EXPECT_EQ(0, c.version());
  EXPECT_EQ(0, d.version());
}

TEST(VersionTest, InitWithAny) {
  Bool a = any<bool>("A");
  Char b = any<char>("B");
  Int c = any<int>("C");

  EXPECT_EQ(0, a.version());
  EXPECT_EQ(0, b.version());
  EXPECT_EQ(0, c.version());
}

TEST(VersionTest, InitWithVar) {
  Int a = 3;
  Int b = a;

  EXPECT_EQ(0, a.version());
  EXPECT_EQ(0, b.version());
}

TEST(VersionTest, InitWithVarRequiringCast) {
  Bool a = any<bool>("A");
  Char b = any<char>("B");
  Int c = b;

  EXPECT_EQ(0, a.version());
  EXPECT_EQ(0, b.version());
  EXPECT_EQ(0, c.version());
}

TEST(VersionTest, InitWithValue) {
  Int a = 3;
  Int b = a + 4;

  EXPECT_EQ(0, a.version());
  EXPECT_EQ(0, b.version());
}

TEST(VersionTest, InitWithValueRequiringCast) {
  Int a = make_value<char>('a');

  EXPECT_EQ(0, a.version());
}

TEST(VersionTest, GetVersion) {
  Int a = 3;
  Int b = any<int>("B");
  Char c = 'c';

  EXPECT_EQ(0, a.version());
  EXPECT_EQ(0, b.version());
  EXPECT_EQ(0, c.version());

  a = b + 2 + c;

  EXPECT_EQ(1, a.version());
  EXPECT_EQ(0, b.version());
  EXPECT_EQ(0, c.version());

  a = 4 + b + c;

  EXPECT_EQ(2, a.version());
  EXPECT_EQ(0, b.version());
  EXPECT_EQ(0, c.version());

  c = a + b;

  EXPECT_EQ(2, a.version());
  EXPECT_EQ(0, b.version());
  EXPECT_EQ(1, c.version());

  b = 5;

  EXPECT_EQ(2, a.version());
  EXPECT_EQ(1, b.version());
  EXPECT_EQ(1, c.version());

  b = a;

  EXPECT_EQ(2, a.version());
  EXPECT_EQ(2, b.version());
  EXPECT_EQ(1, c.version());

  b = c;

  EXPECT_EQ(2, a.version());
  EXPECT_EQ(3, b.version());
  EXPECT_EQ(1, c.version());
}

TEST(VersionTest, SelfAssignmentAfterInit) {
  Int a = 3;
  Int b = any<int>("B");
  Char c = 'c';

  EXPECT_EQ(0, a.version());
  EXPECT_EQ(0, b.version());
  EXPECT_EQ(0, c.version());

  a = a;
  b = b;
  c = c;

  EXPECT_EQ(0, a.version());
  EXPECT_EQ(0, b.version());
  EXPECT_EQ(0, c.version());
}


TEST(VersionTest, SelfAssignmentAfterOperations) {
  Int a = 3;
  Int b = any<int>("B");
  Char c = 'c';

  EXPECT_EQ(0, a.version());
  EXPECT_EQ(0, b.version());
  EXPECT_EQ(0, c.version());

  // some operations
  a = a + b + c;
  b = a + b + c;
  b = 5;
  c = c + 'x';
  c = 'y';
  c = 'z' + c;

  EXPECT_EQ(1, a.version());
  EXPECT_EQ(2, b.version());
  EXPECT_EQ(3, c.version());

  a = a;
  b = b;
  c = c;

  EXPECT_EQ(1, a.version());
  EXPECT_EQ(2, b.version());
  EXPECT_EQ(3, c.version());
}

