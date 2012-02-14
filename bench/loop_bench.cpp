#include "var.h"
#include <climits>

int main(void) {
  Int k = 0;
  for(Int n = 0; n < INT_MAX/8; n = n + 1) {
    if(k == 5) {
      k = 0;
    }
    k = k + 1;
  }
  return 0;
}

