#include <stdio.h>
#include <nonstd.h>

char data[4 * 4096];

// exits with 3 pages swapped out: the kernel must free their slots
int main()
{
  char* pages = (char*) (((size_t) data + 4095) & ~4095UL);
  for (int p = 0; p < 3; ++p)
  {
    pages[p * 4096] = 1;
    swapout(pages + p * 4096);
  }
  printf("[INFO] exiting with %zu slots in use\n", swapinfo());
  return 0;
}
