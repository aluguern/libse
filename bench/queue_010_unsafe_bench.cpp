// Adapted from the SV-COMP'13 benchmark:
//   https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/queue_unsafe.c

#include "libse.h"
#include "concurrent/mutex.h"

using namespace se::ops;

#define N	(10)

#define EMPTY	(1)
#define FALSE	(0)
#define TRUE	(1)

se::Slicer slicer;

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
  if (slicer.begin_then_branch(__COUNTER__, q->head == q->tail)) {
    status = EMPTY;
  }
  slicer.end_branch(__COUNTER__);
  return status;
}

void enqueue(QType *q, se::LocalVar<int> x) {
  q->element[q->tail] = x;
  q->amount = q->amount + 1;
  if (slicer.begin_then_branch(__COUNTER__, q->tail == static_cast<size_t>(N))) {
    q->tail = static_cast<size_t>(1);
  }
  if (slicer.begin_else_branch(__COUNTER__)) {
    q->tail = q->tail + static_cast<size_t>(1);
  }
  slicer.end_branch(__COUNTER__);
}

se::LocalVar<int> dequeue(QType *q) {
  se::LocalVar<int> x;

  x = q->element[q->head];
  q->amount = q->amount - 1;
  if (slicer.begin_then_branch(__COUNTER__, q->head == static_cast<size_t>(N))) {
    q->head = static_cast<size_t>(1);
  }
  if (slicer.begin_else_branch(__COUNTER__)) {
    q->head = q->head + static_cast<size_t>(1);
  }
  slicer.end_branch(__COUNTER__);

  return x;
}

void f1() {
  se::LocalVar<int> v;

  mutex.lock();
  v = se::any<int>();
  enqueue(&queue, v);
  stored_elements[0] = v;
  mutex.unlock();

  for (int i = 1; i < N; i = i + 1) {
    mutex.lock();
    if (slicer.begin_then_branch(__COUNTER__, enqueue_flag == TRUE)) {
      v = se::any<int>();
      enqueue(&queue, v);
      stored_elements[i] = v;
      enqueue_flag = FALSE;
      dequeue_flag = TRUE;
    }
    slicer.end_branch(__COUNTER__);
    mutex.unlock();
  }
}

void f2() {
  for (int i = 0; i < N; i = i + 1) {
    mutex.lock();
    if (slicer.begin_then_branch(__COUNTER__, dequeue_flag == TRUE)) {
      se::LocalVar<int> stored_element;
      stored_element = stored_elements[i];
      se::Thread::error(!(dequeue(&queue) == stored_element));
      dequeue_flag = FALSE;
      enqueue_flag = TRUE;
    }
    slicer.end_branch(__COUNTER__);
    mutex.unlock();
  }
}

int main(void) {
  slicer.begin_slice_loop();
  do {
    se::Thread::encoders().reset();

    init(&queue);

    se::Thread t1(f1);
    se::Thread t2(f2);

    if (se::Thread::encode() && smt::sat == se::Thread::encoders().solver.check()) {
      return 0;
    }
  } while (slicer.next_slice());

  return 1;
}
