#include <stdio.h>
#include <nonstd.h>

char data[71 * 4096];

int main()
{
  char* pages = (char*) (((size_t) data + 4095) & ~4095UL);
  for (int p = 0; p < 70; ++p)
    pages[p * 4096] = (char) p;
  int swapped = 0;
  for (int p = 0; p < 70; ++p)
    swapped += swapout(pages + p * 4096) == 0;
  printf("[INFO] %d of 70 pages swapped out\n", swapped);
  printf(swapped == 64 ? "[PASS] exactly 64 heap slots, then swapout fails\n" : "[FAIL] wrong slot limit\n");
  int ok = 1;
  for (int p = 0; p < 70; ++p)
    ok = ok && pages[p * 4096] == (char) p;
  printf(ok ? "[PASS] all pages correct afterwards\n" : "[FAIL] content lost\n");
  printf(swapinfo() == 0 ? "[PASS] all heap buffers freed again\n" : "[FAIL] heap buffers left\n");
  return 0;
}
