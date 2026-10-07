#include <stdio.h>
#include <nonstd.h>
#include "../../common/include/kernel/syscall-definitions.h"
#include <sys/syscall.h>

char big[65 * 4096];                       // .bss: not in the file, all zero

int main()
{
  char* pages = (char*) (((size_t) big + 4095) & ~4095UL);
  size_t free0 = freepages();
  size_t sum = 0;
  for (int p = 0; p < 64; ++p)
    sum += pages[p * 4096];                // read every page: all map the ONE zero page
  size_t free1 = freepages();
  printf("[INFO] reading 64 zero pages used %zu physical pages\n", free0 - free1);
  printf(sum == 0 ? "[PASS] all pages read as zero\n" : "[FAIL] not zero\n");
  printf(free0 - free1 < 8 ? "[PASS] reading did not allocate 64 pages\n" : "[FAIL] reading allocated pages\n");

  for (int p = 0; p < 32; ++p)
    pages[p * 4096 + 7] = (char) (p + 1);  // write 32 of them: copy on write
  size_t free2 = freepages();
  printf("[INFO] writing 32 of them used %zu physical pages\n", free1 - free2);
  printf(free1 - free2 >= 32 ? "[PASS] each written page got its own physical page\n" : "[FAIL] no copies made\n");
  int ok = 1;
  for (int p = 0; p < 32; ++p)
    ok = ok && pages[p * 4096 + 7] == (char) (p + 1) && pages[p * 4096] == 0;
  for (int p = 32; p < 64; ++p)
    ok = ok && pages[p * 4096 + 7] == 0;
  printf(ok ? "[PASS] written pages keep their values, the others are still zero\n" : "[FAIL] wrong content\n");

  // the KERNEL writes into a zero page (pseudols fills a user buffer): copy on write in kernel mode
  char* buffer = pages + 40 * 4096;
  __syscall(sc_pseudols, (size_t) "/", (size_t) buffer, 256, 0, 0);
  printf(buffer[0] != 0 ? "[PASS] a syscall can write into a zero page\n" : "[FAIL] kernel write lost\n");
  printf(pages[41 * 4096] == 0 && pages[39 * 4096 + 7] == 0 ? "[PASS] the shared zero page is still all zero\n"
                                                            : "[FAIL] the zero page was modified!\n");
  return 0;
}
