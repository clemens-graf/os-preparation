#include <stdio.h>
#include <nonstd.h>

int a = 5;                          // the example from the task description
char other[2 * 4096];

int main()
{
  printf("%d, ", a);
  int ok = createPageBackup(&a) == 0;
  a = 6;
  printf("%d, ", a);
  ok = ok && loadPageBackup(&a) == 0;
  printf("%d\n", a);
  printf(ok && a == 5 ? "[PASS] output 5, 6, 5\n" : "[FAIL] backup not restored\n");

  size_t slots = swapinfo();
  a = 7;
  createPageBackup(&a);             // overwrite the backup ...
  printf(swapinfo() == slots ? "[PASS] a second backup of the page uses no extra slot\n" : "[FAIL] extra slot used\n");
  a = 8;
  loadPageBackup(&a);
  printf(a == 7 ? "[PASS] the newer backup is loaded\n" : "[FAIL] old backup loaded\n");
  a = 9;
  loadPageBackup(&a);
  printf(a == 7 ? "[PASS] a backup can be loaded more than once\n" : "[FAIL] backup gone after loading\n");

  char* page = (char*) (((size_t) other + 4095) & ~4095UL);
  printf(loadPageBackup(page) == -1 ? "[PASS] loading without a backup fails\n" : "[FAIL] load without backup\n");
  printf(createPageBackup((void*) 0x500000000000ULL) == -1 ? "[PASS] unmapped page rejected\n"
                                                           : "[FAIL] unmapped page accepted\n");
  return 0;                         // exits with a backup: its slot must be freed
}
