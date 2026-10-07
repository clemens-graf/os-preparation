#include <stdio.h>
#include <nonstd.h>

int global_variable = 1;
char untouched[3 * 4096];

int main()
{
  int local = 0;
  global_variable++;                      // pages are loaded lazily: touch it first
  ssize_t data_ppn = vtop(&global_variable);
  ssize_t stack_ppn = vtop(&local);
  printf("[INFO] data page -> ppn %zd, stack page -> ppn %zd\n", data_ppn, stack_ppn);
  printf(data_ppn > 0 && stack_ppn > 0 && data_ppn != stack_ppn ? "[PASS] two mapped pages, two physical pages\n"
                                                                : "[FAIL] vtop of mapped pages\n");
  size_t page = (size_t) &global_variable & ~4095UL;
  printf((size_t) ptov(data_ppn) == page ? "[PASS] ptov(vtop(x)) is the page of x\n" : "[FAIL] ptov wrong\n");
  printf(vtop((char*) &global_variable + 1) == data_ppn ? "[PASS] same page, same ppn\n" : "[FAIL] offset changes ppn\n");

  char* fresh = (char*) (((size_t) untouched + 4095) & ~4095UL);
  printf(vtop(fresh) == -1 ? "[PASS] not yet loaded page has no ppn\n" : "[FAIL] unloaded page has a ppn\n");
  fresh[0] = 1;
  printf(vtop(fresh) > 0 ? "[PASS] after the first access it has one\n" : "[FAIL] still no ppn\n");

  printf(vtop((void*) 0xFFFFFFFF80000000ULL) == -1 ? "[PASS] kernel address rejected\n" : "[FAIL] kernel address\n");
  printf(ptov(1) == 0 ? "[PASS] a kernel page is not mapped in this process\n" : "[FAIL] ptov(1)\n");
  printf(ptov(1u << 30) == 0 ? "[PASS] nonexistent ppn rejected\n" : "[FAIL] huge ppn\n");
  return 0;
}
