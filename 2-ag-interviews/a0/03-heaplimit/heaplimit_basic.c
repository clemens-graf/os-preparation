#include <stdio.h>
#include <nonstd.h>

int main()
{
  printf(heaplimit(1025 * 4096) == -1 ? "[PASS] too large rejected\n" : "[FAIL] too large accepted\n");
  printf(heaplimit(10 * 4096) == 0 ? "[PASS] heap of 10 pages\n" : "[FAIL] heaplimit failed\n");

  for (int i = 0; i < 10 * 4096; ++i)
    HEAP_START[i] = (char) i;
  int ok = 1;
  for (int i = 0; i < 10 * 4096; ++i)
    ok = ok && HEAP_START[i] == (char) i;
  printf(ok ? "[PASS] all 10 pages usable and keep their content\n" : "[FAIL] heap content wrong\n");

  heaplimit(2 * 4096);                               // shrink: pages 2..9 are gone
  heaplimit(10 * 4096);                              // grow again: they come back zeroed
  printf(HEAP_START[0] == 0 && HEAP_START[4096 + 1] == 1 ? "[PASS] pages below the limit kept\n"
                                                         : "[FAIL] pages below the limit lost\n");
  printf(HEAP_START[5 * 4096] == 0 ? "[PASS] page above the old limit is new and zeroed\n"
                                   : "[FAIL] shrinking did not unmap\n");

  heaplimit(4096);
  printf("[INFO] accessing above the limit - the kernel must kill this process\n");
  HEAP_START[4096] = 1;
  printf("[FAIL] access above the limit worked\n");
  return 0;
}
