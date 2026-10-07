#include <stdio.h>
#include <sched.h>
#include <nonstd.h>

char data[11 * 4096];

// start it, then press F3 while it runs (tools/run_test.sh '&fkey_swap_check.sweb' '!sleep 1' '!key f3' help)
int main()
{
  char* pages = (char*) (((size_t) data + 4095) & ~4095UL);
  for (int p = 0; p < 10; ++p)
    pages[p * 4096] = (char) (p + 1);
  printf("[INFO] waiting for F3 ...\n");
  size_t used = 0;
  for (int i = 0; i < 200 && used == 0; ++i)    // about 10 seconds
  {
    sched_yield();                              // every syscall may be the one that handles F3
    used = swapinfo();
  }
  printf(used >= 10 ? "[PASS] F3 swapped out this process' pages\n" : "[FAIL] nothing swapped out\n");
  int ok = 1;
  for (int p = 0; p < 10; ++p)
    ok = ok && pages[p * 4096] == (char) (p + 1);
  printf(ok ? "[PASS] and they come back intact\n" : "[FAIL] content lost\n");
  return 0;
}
