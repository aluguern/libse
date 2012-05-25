#include <climits>
#include "sp.h"

int main(void) {
  sp::Int k = 0;
  for(sp::Int n = 0; n < INT_MAX; n = n + 1) {
    if(k == 7) {
      k = 0;
    }
    k = k + 1;
  }

  // force k to be live
  return k != INT_MAX % 7;
}

