#include <climits>
#include "sequential-se.h"

int main(void) {
  se::Int k = 0;
  for(se::Int n = 0; n < INT_MAX; n = n + 1) {
    if(k == 7) {
      k = 0;
    }
    k = k + 1;
  }

  // force k to be live
  return k != INT_MAX % 7;
}

