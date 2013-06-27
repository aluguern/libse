#include "concurrent/mutex.h"
#include "gtest/gtest.h"

using namespace se;

/// Makes Mutex::unlock(Encoders&) public
class InternalMutex : public Mutex {
public:
  InternalMutex() : Mutex() {}
  void unlock(Encoders& encoders) { return Mutex::unlock(encoders); }
};

TEST(MutexTest, SatMainThreadSingleWriter) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();
  SharedVar<int> shared_var;

  shared_var = 1;

  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::sat, encoders.solver.check());
}

TEST(MutexTest, SatSingleWriter) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> shared_var;

  Threads::begin_thread();
  shared_var = shared_var + 3;
  shared_var = shared_var + 1;
  Threads::end_thread();

  Threads::begin_thread();
  Threads::error(shared_var == 3, encoders);
  Threads::end_thread();

  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::sat, encoders.solver.check());
}

TEST(MutexTest, UnsatSingleWriter) {
  Encoders encoders;

  Threads::reset();
  Threads::begin_main_thread();

  SharedVar<int> shared_var;
  InternalMutex mutex;

  Threads::begin_thread();
  mutex.lock();
  shared_var = shared_var + 3;
  shared_var = shared_var + 1;
  mutex.unlock(encoders);
  Threads::end_thread();

  Threads::begin_thread();
  mutex.lock();
  Threads::error(shared_var == 3, encoders);
  mutex.unlock(encoders);
  Threads::end_thread();

  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::unsat, encoders.solver.check());
}

TEST(MutexTest, Sat1MultipleWriters) {
  Encoders encoders;

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
  mutex.unlock(encoders);

  Threads::end_thread();

  Threads::begin_thread();

  mutex.lock();
  x = '\1';
  y = '\2';
  z = '\3';
  mutex.unlock(encoders);

  Threads::end_thread();

  // Main thread does not use lock and therefore fails!
  a = x;
  b = y;
  c = z;

  std::unique_ptr<ReadInstr<bool>> c0(a == 'A' && b == '\2' && c == 'C');
  std::unique_ptr<ReadInstr<bool>> c1(a == '\1' && b == 'B' && c == '\3');

  Threads::end_main_thread(encoders);

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(MutexTest, Sat2MultipleWriters) {
  Encoders encoders;

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

  Threads::end_thread();

  Threads::begin_thread();

  x = '\1';
  y = '\2';
  z = '\3';

  Threads::end_thread();

  a = x;
  b = y;
  c = z;

  std::unique_ptr<ReadInstr<bool>> c0(a == 'A' && b == '\2' && c == 'C');
  std::unique_ptr<ReadInstr<bool>> c1(a == '\1' && b == 'B' && c == '\3');

  Threads::end_main_thread(encoders);

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::sat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(MutexTest, UnsatMultipleWriters) {
  Encoders encoders;

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
  mutex.unlock(encoders);

  Threads::end_thread();

  Threads::begin_thread();

  mutex.lock();
  x = '\1';
  y = '\2';
  z = '\3';
  mutex.unlock(encoders);

  Threads::end_thread();

  mutex.lock();
  a = x;
  mutex.unlock(encoders);

  mutex.lock();
  b = y;
  mutex.unlock(encoders);

  mutex.lock();
  c = z;
  mutex.unlock(encoders);

  std::unique_ptr<ReadInstr<bool>> c0(a == 'A' && b == '\2' && c == 'C');
  std::unique_ptr<ReadInstr<bool>> c1(a == '\1' && b == 'B' && c == '\3');

  Threads::end_main_thread(encoders);

  encoders.solver.push();

  Threads::internal_error(std::move(c0), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();

  encoders.solver.push();

  Threads::internal_error(std::move(c1), encoders);
  EXPECT_EQ(smt::unsat, encoders.solver.check());

  encoders.solver.pop();
}

TEST(MutexTest, SatJoinMultipleWriters) {
  Encoders encoders;

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
  mutex.unlock(encoders);

  mutex.lock();
  y = y + 1;
  mutex.unlock(encoders);

  t1_send_event_ptr = Threads::end_thread();

  Threads::begin_thread();

  mutex.lock();
  x = x + 5;
  mutex.unlock(encoders);

  mutex.lock();
  y = y - 6;
  mutex.unlock(encoders);

  t2_send_event_ptr = Threads::end_thread();

  Threads::join(t1_send_event_ptr);
  Threads::join(t2_send_event_ptr);

  Threads::error(x == 16 && y == 5, encoders);
  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::sat, encoders.solver.check());
}

TEST(MutexTest, UnsatJoinMultipleWriters) {
  Encoders encoders;

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
  mutex.unlock(encoders);

  mutex.lock();
  y = y + 1;
  mutex.unlock(encoders);

  t1_send_event_ptr = Threads::end_thread();

  Threads::begin_thread();

  mutex.lock();
  x = x + 5;
  mutex.unlock(encoders);

  mutex.lock();
  y = y - 6;
  mutex.unlock(encoders);

  t2_send_event_ptr = Threads::end_thread();

  Threads::join(t1_send_event_ptr);
  Threads::join(t2_send_event_ptr);

  Threads::error(!(x == 16) || !(y == 5), encoders);
  Threads::end_main_thread(encoders);

  EXPECT_EQ(smt::unsat, encoders.solver.check());
}
