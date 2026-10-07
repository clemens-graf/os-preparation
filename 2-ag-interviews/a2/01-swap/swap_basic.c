#include <stdio.h>
#include <nonstd.h>

char data[9 * 4096];

int main()
{
  char* pages = (char*) (((size_t) data + 4095) & ~4095UL);
  for (int p = 0; p < 8; ++p)
    for (int i = 0; i < 4096; ++i)
      pages[p * 4096 + i] = (char) (p * 31 + i);

  size_t before = swapinfo();
  int ok = 1;
  for (int p = 0; p < 8; ++p)
    ok = ok && swapout(pages + p * 4096) == 0;
  printf(ok ? "[PASS] 8 pages swapped out\n" : "[FAIL] swapout failed\n");
  printf(swapinfo() == before + 8 ? "[PASS] 8 swap slots in use\n" : "[FAIL] wrong number of slots\n");
  printf(swapout(pages) == -1 ? "[PASS] a swapped-out page cannot be swapped out again\n" : "[FAIL] swapped twice\n");

  ok = 1;
  for (int p = 0; p < 8; ++p)
    for (int i = 0; i < 4096; ++i)
      ok = ok && pages[p * 4096 + i] == (char) (p * 31 + i);
  printf(ok ? "[PASS] all 8 pages came back with their content\n" : "[FAIL] content lost\n");
  printf(swapinfo() == before ? "[PASS] slots freed on swap-in\n" : "[FAIL] slots not freed\n");

  int local = 42;
  swapout(&local);                          // our own stack page ...
  printf(local == 42 ? "[PASS] the stack page comes back on the next access\n" : "[FAIL] stack lost\n");
  swapout((void*) main);                    // ... and our own code
  printf("[PASS] the code page comes back too\n");

  printf(swapout((void*) 0x500000000000ULL) == -1 ? "[PASS] unmapped page rejected\n" : "[FAIL] unmapped accepted\n");
  printf(swapout((void*) 0xFFFFFFFF80000000ULL) == -1 ? "[PASS] kernel address rejected\n" : "[FAIL] kernel address\n");
  return 0;
}
