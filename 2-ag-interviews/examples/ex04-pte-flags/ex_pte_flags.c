#include <stdio.h>
#include <nonstd.h>

char buffer[3 * 4096];

int main()
{
  char* page = (char*) (((size_t) buffer + 4095) & ~4095UL); // a page-aligned address inside buffer
  printf(protectpage(page, 0) == -1 ? "[PASS] unmapped page rejected\n" : "[FAIL] unmapped page accepted\n");
  page[0] = 'x';                                               // now it is mapped (loaded on demand)

  printf(protectpage(page, 0) == 0 ? "[PASS] made read-only\n" : "[FAIL] protectpage failed\n");
  printf(page[0] == 'x' ? "[PASS] reading still works\n" : "[FAIL] read failed\n");
  protectpage(page, 1);
  page[1] = 'y';
  printf(page[1] == 'y' ? "[PASS] writable again\n" : "[FAIL] write failed\n");

  protectpage(page, 0);
  printf("[INFO] writing to the read-only page now - the kernel must kill this process\n");
  page[2] = 'z';
  printf("[FAIL] the write to a read-only page succeeded\n");
  return 0;
}
