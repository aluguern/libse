/* 2^16 */
static int N = 65536;

int main(void) {
  int k = 0;
  for(int n = 0; n < N; n = n + 1) {
    if(k == 7) {
      k = 0;
    }
    k = k + 1;
  }

  // force k to be live
  return k != N % 7;
}

