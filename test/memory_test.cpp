#include <sstream>
#include "gtest/gtest.h"
#include "memory.h"

using namespace se;

TEST(MemoryTest, InitialVersion) {
  const Memory<int> memory(5, "F");
  EXPECT_EQ(0, memory.version());
}

TEST(MemoryTest, InitialSelectWithConcreteIndex) {
  const Memory<int> memory(5, "F");

  std::stringstream out;
  out << memory.load_value(2).expr();

  EXPECT_EQ("Select([F], 2)", out.str());
  EXPECT_EQ(0, memory.version());
}

TEST(MemoryTest, InitialSelectWithSymbolicIndex) {
  const Value<size_t> i = any<size_t>("I");
  const Memory<int> memory(5, "F");

  std::stringstream out;
  out << memory.load_value(i).expr();

  EXPECT_EQ("Select([F], [I])", out.str());
  EXPECT_EQ(0, memory.version());
}

TEST(MemoryTest, InitialStoreWithConcreteIndex) {
  Memory<int> memory(5, "F");
  const Value<int> x = any<int>("X");

  memory.store_value(2, x);

  std::stringstream out;
  out << memory.load_value(2).expr();

  EXPECT_EQ("Select(Store([F], 2, [X]), 2)", out.str());
  EXPECT_EQ(1, memory.version());
}

TEST(MemoryTest, InitialStoreWithSymbolicIndex) {
  const Value<size_t> i = any<size_t>("I");
  const Value<int> x = any<int>("X");
  Memory<int> memory(5, "F");

  memory.store_value(i, x);

  std::stringstream out;
  out << memory.load_value(i).expr();

  EXPECT_EQ("Select(Store([F], [I], [X]), [I])", out.str());
  EXPECT_EQ(1, memory.version());
}

TEST(MemoryTest, LoadMemoryBlockWithConcreteIndex) {
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  const MemoryBlock<int> block(memory, 2);

  std::stringstream out;
  out << block.value().expr();

  EXPECT_EQ("Select([F], 2)", out.str());
  EXPECT_EQ(0, memory->version());
}

TEST(MemoryTest, LoadMemoryBlockWithSymbolicIndex) {
  const Value<size_t> i = any<size_t>("I");
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  const MemoryBlock<int> block(memory, i);

  std::stringstream out;
  out << block.value().expr();

  EXPECT_EQ("Select([F], [I])", out.str());
  EXPECT_EQ(0, memory->version());
}

TEST(MemoryTest, StoreMemoryBlockWithConcreteIndex) {
  const Var<int> x = any<int>("X");
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  MemoryBlock<int> block(memory, 2);

  block = x;

  std::stringstream out;
  out << block.value().expr();

  EXPECT_EQ("Select(Store([F], 2, [X]), 2)", out.str());
  EXPECT_EQ(1, memory->version());
}

TEST(MemoryTest, StoreMemoryBlockWithSymbolicIndex) {
  const Value<size_t> i = any<size_t>("I");
  const Var<int> x = any<int>("X");
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  MemoryBlock<int> block(memory, i);

  block = x;

  std::stringstream out;
  out << block.value().expr();

  EXPECT_EQ("Select(Store([F], [I], [X]), [I])", out.str());
  EXPECT_EQ(1, memory->version());
}

TEST(MemoryTest, SelectValueWithConcreteIndex) {
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  const Value<int*> array(memory);

  std::stringstream out;
  out << array[2].value().expr();

  EXPECT_EQ("Select([F], 2)", out.str());
  EXPECT_EQ(0, memory->version());
}

TEST(MemoryTest, SelectValueWithSymbolicIndex) {
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  const Value<int*> array(memory);
  const Value<size_t> i = any<size_t>("I");

  std::stringstream out;
  out << array[i].value().expr();

  EXPECT_EQ("Select([F], (0+[I]))", out.str());
  EXPECT_EQ(0, memory->version());
}

TEST(MemoryTest, SelectValueWithSymbolicVarIndex) {
  const Var<size_t> i = any<size_t>("I");
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  const Value<int*> array(memory);

  std::stringstream out;
  out << array[i].value().expr();

  EXPECT_EQ("Select([F], (0+[I]))", out.str());
  EXPECT_EQ(0, memory->version());
}

TEST(MemoryTest, StoreValueWithConcreteIndex) {
  const Value<int> x = any<int>("X");
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  const Value<int*> array(memory);

  array[2] = x;

  std::stringstream out;
  out << array[2].value().expr();

  EXPECT_EQ("Select(Store([F], 2, [X]), 2)", out.str());
  EXPECT_EQ(1, memory->version());
}

TEST(MemoryTest, StoreValueWithSymbolicIndex) {
  const Value<size_t> i = any<size_t>("I");
  const Value<int> x = any<int>("X");
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  const Value<int*> array(memory);

  array[i] = x;

  std::stringstream out;
  out << array[i].value().expr();

  EXPECT_EQ("Select(Store([F], (0+[I]), [X]), (0+[I]))", out.str());
  EXPECT_EQ(1, memory->version());
}

TEST(MemoryTest, SelectVarWithConcreteIndex) {
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  const Value<int*> array_value(memory);
  const Var<int*> array(array_value);

  std::stringstream out;
  out << array[2].value().expr();

  EXPECT_EQ("Select([F], 2)", out.str());
  EXPECT_EQ(0, memory->version());
}

TEST(MemoryTest, SelectVarWithSymbolicIndex) {
  const Value<size_t> i = any<size_t>("I");
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  const Value<int*> array_value(memory);
  const Var<int*> array(array_value);

  std::stringstream out;
  out << array[i].value().expr();

  EXPECT_EQ("Select([F], (0+[I]))", out.str());
  EXPECT_EQ(0, memory->version());
}

TEST(MemoryTest, StoreVarWithConcreteIndex) {
  const Value<int> x = any<int>("X");
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  const Value<int*> array(memory);

  array[2] = x;

  std::stringstream out;
  out << array[2].value().expr();

  EXPECT_EQ("Select(Store([F], 2, [X]), 2)", out.str());
  EXPECT_EQ(1, memory->version());
}

TEST(MemoryTest, StoreVarWithSymbolicIndex) {
  const Value<size_t> i = any<size_t>("I");
  const Value<int> x = any<int>("X");
  const std::shared_ptr<Memory<int>> memory(new Memory<int>(5, "F"));
  const Value<int*> array(memory);

  array[i] = x;

  std::stringstream out;
  out << array[i].value().expr();

  EXPECT_EQ("Select(Store([F], (0+[I]), [X]), (0+[I]))", out.str());
  EXPECT_EQ(1, memory->version());
}

TEST(MemoryTest, MallocWithValueIndex) {
  const Value<size_t> i = any<size_t>("I");
  const Value<int> x = any<int>("X");
  const Value<int> y = any<int>("Y");
  const Value<int*> array = malloc<int>(5, "F");

  array[i] = x;
  array[2] = y;

  std::stringstream out;
  out << array[i].value().expr();

  EXPECT_EQ("Select(Store(Store([F], (0+[I]), [X]), 2, [Y]), (0+[I]))", out.str());
}

TEST(MemoryTest, MallocWithVarIndex) {
  const Var<size_t> i = any<size_t>("I");
  const Var<int> x = any<int>("X");
  const Var<int> y = any<int>("Y");
  const Var<int*> array = malloc<int>(5, "F");

  array[i] = x;
  array[2] = y;

  std::stringstream out;
  out << array[i].value().expr();

  EXPECT_EQ("Select(Store(Store([F], (0+[I]), [X]), 2, [Y]), (0+[I]))", out.str());
}
