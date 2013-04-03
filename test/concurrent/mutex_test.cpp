#include "gtest/gtest.h"

#include "concurrent/mutex.h"

using namespace se;

/// Makes Mutex::unlock(Z3&) public
class InternalMutex : public Mutex {
public:
  InternalMutex() : Mutex() {}
  void unlock(Z3& z3) { return Mutex::unlock(z3); }
};

TEST(MutexTest, SatSingleWriter) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> shared_var;

  Threads::begin_thread();
  shared_var = shared_var + 3;
  shared_var = shared_var + 1;
  Threads::end_thread(z3);

  Threads::begin_thread();
  Threads::error(shared_var == 3, z3);
  Threads::end_thread(z3);

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::sat, z3.solver.check());
}

TEST(MutexTest, UnsatSingleWriter) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> shared_var;
  InternalMutex mutex;

  Threads::begin_thread();
  mutex.lock();
  shared_var = shared_var + 3;
  shared_var = shared_var + 1;
  mutex.unlock(z3);
  Threads::end_thread(z3);

  Threads::begin_thread();
  mutex.lock();
  Threads::error(shared_var == 3, z3);
  mutex.unlock(z3);
  Threads::end_thread(z3);

  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::unsat, z3.solver.check());
}

TEST(MutexTest, Sat1MultipleWriters) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  SharedVar<char> y;
  SharedVar<char> z;

  LocalVar<char> a;
  LocalVar<char> b;
  LocalVar<char> c;

  InternalMutex mutex;

  Threads::begin_thread();

  mutex.lock();
  x = 'A';
  y = 'B';
  z = 'C';
  mutex.unlock(z3);

  Threads::end_thread(z3);

  Threads::begin_thread();

  mutex.lock();
  x = '\1';
  y = '\2';
  z = '\3';
  mutex.unlock(z3);

  Threads::end_thread(z3);

  // Main thread does not use lock and therefore fails!
  a = x;
  b = y;
  c = z;

  std::unique_ptr<ReadInstr<bool>> c0(a == 'A' && b == '\2' && c == 'C');
  std::unique_ptr<ReadInstr<bool>> c1(a == '\1' && b == 'B' && c == '\3');

  Threads::end_main_thread(z3);

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();
}

TEST(MutexTest, Sat2MultipleWriters) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  SharedVar<char> y;
  SharedVar<char> z;

  LocalVar<char> a;
  LocalVar<char> b;
  LocalVar<char> c;

  InternalMutex mutex;

  Threads::begin_thread();

  x = 'A';
  y = 'B';
  z = 'C';

  Threads::end_thread(z3);

  Threads::begin_thread();

  x = '\1';
  y = '\2';
  z = '\3';

  Threads::end_thread(z3);

  a = x;
  b = y;
  c = z;

  std::unique_ptr<ReadInstr<bool>> c0(a == 'A' && b == '\2' && c == 'C');
  std::unique_ptr<ReadInstr<bool>> c1(a == '\1' && b == 'B' && c == '\3');

  Threads::end_main_thread(z3);

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::sat, z3.solver.check());

  z3.solver.pop();
}

TEST(MutexTest, UnsatMultipleWriters) {
  Z3 z3;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<char> x;
  SharedVar<char> y;
  SharedVar<char> z;

  LocalVar<char> a;
  LocalVar<char> b;
  LocalVar<char> c;

  InternalMutex mutex;

  Threads::begin_thread();

  mutex.lock();
  x = 'A';
  y = 'B';
  z = 'C';
  mutex.unlock(z3);

  Threads::end_thread(z3);

  Threads::begin_thread();

  mutex.lock();
  x = '\1';
  y = '\2';
  z = '\3';
  mutex.unlock(z3);

  Threads::end_thread(z3);

  mutex.lock();
  a = x;
  mutex.unlock(z3);

  mutex.lock();
  b = y;
  mutex.unlock(z3);

  mutex.lock();
  c = z;
  mutex.unlock(z3);

  std::unique_ptr<ReadInstr<bool>> c0(a == 'A' && b == '\2' && c == 'C');
  std::unique_ptr<ReadInstr<bool>> c1(a == '\1' && b == 'B' && c == '\3');

  Threads::end_main_thread(z3);

  z3.solver.push();

  Threads::internal_error(std::move(c0), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();

  z3.solver.push();

  Threads::internal_error(std::move(c1), z3);
  EXPECT_EQ(z3::unsat, z3.solver.check());

  z3.solver.pop();
}

TEST(MutexTest, SatJoinMultipleWriters) {
  Z3 z3;

  std::shared_ptr<SendEvent> t1_send_event_ptr;
  std::shared_ptr<SendEvent> t2_send_event_ptr;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> x = 10;
  SharedVar<int> y = 10;
  InternalMutex mutex;

  Threads::begin_thread();

  mutex.lock();
  x = x + 1;
  mutex.unlock(z3);

  mutex.lock();
  y = y + 1;
  mutex.unlock(z3);

  t1_send_event_ptr = Threads::end_thread(z3);

  Threads::begin_thread();

  mutex.lock();
  x = x + 5;
  mutex.unlock(z3);

  mutex.lock();
  y = y - 6;
  mutex.unlock(z3);

  t2_send_event_ptr = Threads::end_thread(z3);

  Threads::join(t1_send_event_ptr);
  Threads::join(t2_send_event_ptr);

  Threads::error(x == 16 && y == 5, z3);
  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::sat, z3.solver.check());
}

TEST(MutexTest, UnsatJoinMultipleWriters) {
  Z3 z3;

  std::shared_ptr<SendEvent> t1_send_event_ptr;
  std::shared_ptr<SendEvent> t2_send_event_ptr;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> x = 10;
  SharedVar<int> y = 10;
  InternalMutex mutex;

  Threads::begin_thread();

  mutex.lock();
  x = x + 1;
  mutex.unlock(z3);

  mutex.lock();
  y = y + 1;
  mutex.unlock(z3);

  t1_send_event_ptr = Threads::end_thread(z3);

  Threads::begin_thread();

  mutex.lock();
  x = x + 5;
  mutex.unlock(z3);

  mutex.lock();
  y = y - 6;
  mutex.unlock(z3);

  t2_send_event_ptr = Threads::end_thread(z3);

  Threads::join(t1_send_event_ptr);
  Threads::join(t2_send_event_ptr);

  Threads::error(!(x == 16) || !(y == 5), z3);
  Threads::end_main_thread(z3);

  EXPECT_EQ(z3::unsat, z3.solver.check());
}
