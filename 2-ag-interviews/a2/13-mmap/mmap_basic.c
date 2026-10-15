#include <stdio.h>
#include <sys/mman.h>

#define FAILED ((void*) -1)
#define RW (PROT_READ | PROT_WRITE)
#define ANON (MAP_PRIVATE | MAP_ANONYMOUS)

int main()
{
  char* a = mmap(0, 3 * 4096, RW, ANON, -1, 0);
  char* b = mmap(0, 5000, RW, ANON, -1, 0);           // rounded up to 2 pages
  printf(a != FAILED && b != FAILED && a != 0 && b != 0 ? "[PASS] two mappings\n" : "[FAIL] mmap failed\n");
  printf(((size_t) a % 4096) == 0 && ((size_t) b % 4096) == 0 ? "[PASS] page aligned\n" : "[FAIL] not aligned\n");
  printf(b >= a + 3 * 4096 || b + 2 * 4096 <= a ? "[PASS] mappings do not overlap\n" : "[FAIL] overlap\n");

  int zero = 1;
  for (int i = 0; i < 3 * 4096; ++i)
    zero = zero && a[i] == 0;
  printf(zero ? "[PASS] new memory is zero\n" : "[FAIL] not zeroed\n");

  for (int i = 0; i < 3 * 4096; ++i)
    a[i] = (char) i;
  b[4999] = 'x';
  int ok = b[4999] == 'x';
  for (int i = 0; i < 3 * 4096; ++i)
    ok = ok && a[i] == (char) i;
  printf(ok ? "[PASS] every page usable, content kept\n" : "[FAIL] content wrong\n");

  printf(mmap(0, 0, RW, ANON, -1, 0) == FAILED ? "[PASS] length 0 rejected\n" : "[FAIL] length 0 accepted\n");
  printf(mmap(0, 4096, RW, MAP_PRIVATE, 3, 0) == FAILED ? "[PASS] file mappings not supported: rejected\n"
                                                        : "[FAIL] file mapping accepted\n");
  printf(munmap(a + 1, 4096) == -1 ? "[PASS] unaligned munmap rejected\n" : "[FAIL] unaligned munmap\n");
  printf(munmap(b, 2 * 4096) == 0 ? "[PASS] munmap\n" : "[FAIL] munmap failed\n");
  printf(munmap(b, 2 * 4096) == -1 ? "[PASS] second munmap rejected\n" : "[FAIL] munmapped twice\n");

  char* c = mmap(0, 4096, RW, ANON, -1, 0);
  printf(c != FAILED && c[0] == 0 ? "[PASS] mapping after munmap works\n" : "[FAIL] mmap after munmap\n");

  munmap(a, 3 * 4096);
  printf("[INFO] touching unmapped memory - the kernel must kill this process\n");
  a[0] = 1;
  printf("[FAIL] unmapped memory still usable\n");
  return 0;
}
