#include <limits.h>

int main(void) {
  int k = nondet_int();

  while(k < INT_MAX) {
    k = k + 1;
  }

  return 0;
}

