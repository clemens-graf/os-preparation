#include <stdio.h>
#include <string.h>
#include <nonstd.h>

int main()
{
  char out[512];
  char in[512];
  memset(out, 0, sizeof(out));
  memset(in, 'x', sizeof(in));
  memcpy(out, "a note on the swap partition", 29); // user libc has no strcpy

  printf(disknote(1, out) == 0 ? "[PASS] block written\n" : "[FAIL] write failed\n");
  printf(disknote(0, in) == 0 ? "[PASS] block read\n" : "[FAIL] read failed\n");
  printf(memcmp(in, out, sizeof(in)) == 0 ? "[PASS] read back what was written\n" : "[FAIL] content differs\n");
  printf(disknote(0, (char*) 0xFFFFFFFF80000000ULL) == -1 ? "[PASS] kernel pointer rejected\n"
                                                          : "[FAIL] kernel pointer accepted\n");
  return 0;
}
