#include <stdio.h>
#include <sys/mman.h>

char data[3 * 4096];

int main()
{
  char* page = (char*) (((size_t) data + 4095) & ~4095UL);
  page[0] = 'a';                                         // pages are loaded lazily: touch first
  printf(mprotect(page, 4096, PROT_READ) == 0 ? "[PASS] made read-only\n" : "[FAIL] mprotect failed\n");
  printf(page[0] == 'a' ? "[PASS] still readable\n" : "[FAIL] not readable\n");
  mprotect(page, 4096, PROT_READ | PROT_WRITE);
  page[1] = 'b';
  printf(page[1] == 'b' ? "[PASS] writable again\n" : "[FAIL] not writable\n");

  printf(mprotect(page, 4096, PROT_WRITE | PROT_EXEC) == -1 ? "[PASS] W xor X: write + exec refused\n"
                                                            : "[FAIL] writable and executable allowed\n");
  printf(mprotect(page + 1, 4096, PROT_READ) == -1 ? "[PASS] unaligned address refused\n" : "[FAIL] unaligned\n");
  printf(mprotect((void*) 0x500000000000ULL, 4096, PROT_READ) == -1 ? "[PASS] unmapped range refused\n"
                                                                    : "[FAIL] unmapped range\n");

  page[0] = (char) 0xC3;                                 // x86 'ret'
  mprotect(page, 4096, PROT_READ | PROT_EXEC);
  ((void (*)(void)) page)();
  printf("[PASS] code on an executable page runs\n");
  mprotect(page, 4096, PROT_READ);
  printf("[INFO] calling code on a non-executable page - the kernel must kill this process\n");
  ((void (*)(void)) page)();
  printf("[FAIL] executed a non-executable page\n");
  return 0;
}
