// Adapted from the SV-COMP'13 benchmark:
//   https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/queue_ok_safe.c

#include "libse.h"
#include "concurrent/mutex.h"

#define N	(10)

#define EMPTY	(1)
#define FALSE	(0)
#define TRUE	(1)

typedef struct {
  se::SharedVar<int[N]> element;
  se::SharedVar<size_t> head;
  se::SharedVar<size_t> tail;
  se::SharedVar<int> amount;
} QType;

se::SharedVar<int[N]> stored_elements;

se::SharedVar<int> enqueue_flag = TRUE;
se::SharedVar<int> dequeue_flag = FALSE;

se::Mutex mutex;
QType queue;

void init(QType *q) {
  q->head = static_cast<size_t>(0);
  q->tail = static_cast<size_t>(0);
  q->amount = 0;
}

se::LocalVar<int> empty(QType *q) {
  se::SharedVar<int> status = 0;
  if (se::ThisThread::recorder().begin_then(q->head == q->tail)) {
    status = EMPTY;
  }
  se::ThisThread::recorder().end_branch();
  return status;
}

void enqueue(QType *q, se::LocalVar<int> x) {
  q->element[q->tail] = x;
  q->amount = q->amount + 1;
  if (se::ThisThread::recorder().begin_then(q->tail == static_cast<size_t>(N))) {
    q->tail = static_cast<size_t>(1);
  }
  if (se::ThisThread::recorder().begin_else()) {
    q->tail = q->tail + static_cast<size_t>(1);
  }
  se::ThisThread::recorder().end_branch();
}

se::LocalVar<int> dequeue(QType *q) {
  se::LocalVar<int> x;

  x = q->element[q->head];
  q->amount = q->amount - 1;
  if (se::ThisThread::recorder().begin_then(q->head == static_cast<size_t>(N))) {
    q->head = static_cast<size_t>(1);
  }
  if (se::ThisThread::recorder().begin_else()) {
    q->head = q->head + static_cast<size_t>(1);
  }
  se::ThisThread::recorder().end_branch();

  return x;
}

void f1() {
  se::LocalVar<int> v;

  mutex.lock();
  if (se::ThisThread::recorder().begin_then(enqueue_flag == TRUE)) {
    for (int i = 0; i < N; i = i + 1) {
      v = se::any<int>();

      enqueue(&queue, v);
      stored_elements[i] = v;
    }

    enqueue_flag = FALSE;
    dequeue_flag = TRUE;
  }
  se::ThisThread::recorder().end_branch();
  mutex.unlock();
}

void f2() {
  mutex.lock();
  if (se::ThisThread::recorder().begin_then(dequeue_flag == TRUE)) {
    for (int i = 0; i < N; i = i + 1) {
      if (se::ThisThread::recorder().begin_then(!(empty(&queue) == EMPTY))) {
        se::LocalVar<int> stored_element;
        stored_element = stored_elements[i];
        se::Thread::error(!(dequeue(&queue) == stored_element));
      }
      se::ThisThread::recorder().end_branch();
    }

    dequeue_flag = FALSE;
    enqueue_flag = TRUE;
  }
  se::ThisThread::recorder().end_branch();
  mutex.unlock();
}

int main(void) {
  init(&queue);

  se::Thread t1(f1);
  se::Thread t2(f2);

  se::Thread::end_main_thread();

  return z3::sat == se::Thread::z3().solver.check();
}
