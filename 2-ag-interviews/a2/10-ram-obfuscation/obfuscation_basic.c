#include <stdio.h>
#include <sched.h>
#include <nonstd.h>

char data[17 * 4096];

int recurse(int depth)
{
  volatile char frame[256];
  for (int i = 0; i < 256; ++i)
    frame[i] = (char) (depth + i);
  int sum = depth == 0 ? 0 : recurse(depth - 1);
  for (int i = 0; i < 256; ++i)
    sum += frame[i] == (char) (depth + i);
  return sum;
}

int main()
{
  char* pages = (char*) (((size_t) data + 4095) & ~4095UL);
  for (int p = 0; p < 16; ++p)
    for (int i = 0; i < 4096; i += 64)
      pages[p * 4096 + i] = (char) (p * 7 + i);
  int errors = 0;
  for (int round = 0; round < 60; ++round)
  {
    for (int p = 0; p < 16; ++p)
      for (int i = 0; i < 4096; i += 64)
        errors += pages[p * 4096 + i] != (char) (p * 7 + i);
    errors += recurse(10) != 11 * 256;      // the stack moves too
    sched_yield();
  }
  size_t moved = relocations();
  printf("[INFO] %zu pages of this process were moved, %d errors\n", moved, errors);
  printf(moved > 0 ? "[PASS] the obfuscation thread moved pages of this process\n" : "[FAIL] nothing moved\n");
  printf(errors == 0 ? "[PASS] no data was lost or corrupted\n" : "[FAIL] data corrupted\n");
  return 0;
}
