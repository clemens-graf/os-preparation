#include <stdio.h>
#include <nonstd.h>

char pages[8 * 4096];
struct pfstats untouched_stats __attribute__((aligned(4096)));   // own page in .bss, not loaded yet

int main()
{
  struct pfstats a, b;
  char* p = (char*) (((size_t) pages + 4095) & ~4095UL);
  pfstats(&a);                       // warm-up: loads the code of pfstats and the stack is there
  pfstats(&a);
  for (int i = 0; i < 4; ++i)
    p[i * 4096] = 1;                 // 4 write faults
  volatile char sum = p[4 * 4096] + p[5 * 4096];   // 2 read faults
  pfstats(&b);
  printf("[INFO] total %zu, user %zu, kernel %zu, writes %zu, reads %zu, fetches %zu, last %zx\n",
         b.total, b.user, b.kernel, b.writes, b.reads, b.fetches, b.last_address);
  printf(b.total - a.total == 6 ? "[PASS] 6 new page faults counted\n" : "[FAIL] wrong number of faults\n");
  printf(b.writes - a.writes == 4 && b.reads - a.reads == 2 ? "[PASS] 4 writes, 2 reads\n"
                                                             : "[FAIL] read/write split wrong\n");
  printf(b.last_address == (size_t) (p + 5 * 4096) ? "[PASS] last address is cr2 of the last fault\n"
                                                   : "[FAIL] wrong last address\n");
  printf(b.fetches >= 1 && b.user >= 6 ? "[PASS] instruction fetches and user faults counted\n"
                                       : "[FAIL] fetch/user counts\n");
  size_t kernel_before = b.kernel;
  pfstats(&untouched_stats);         // the KERNEL writes into a page that is not loaded yet ...
  pfstats(&b);                       // ... (the copy it wrote was taken before that fault)
  printf(b.kernel == kernel_before + 1 ? "[PASS] a kernel-mode fault on a user page is counted\n"
                                                     : "[FAIL] kernel-mode fault not counted\n");
  printf(pfstats((struct pfstats*) 0xFFFFFFFF80000000ULL) == -1 ? "[PASS] kernel pointer rejected\n"
                                                                : "[FAIL] kernel pointer accepted\n");
  return sum;
}
