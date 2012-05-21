#include <limits.h>

const int N = 65536;

int main(void) {
  int k = nondet_int();

  while(k < INT_MAX) {
    k = k + 1;
  }

  return 0;
}

