#include <stdio.h>
#include <nonstd.h>

int main()
{
  char* a = allocpage();
  char* b = allocpage();
  char* c = allocpage();
  printf("[INFO] pages at %p %p %p\n", a, b, c);
  printf(a && b && c && a != b && b != c && a != c ? "[PASS] three different pages\n" : "[FAIL] pages not distinct\n");
  printf(((size_t) a & 4095) == 0 ? "[PASS] page aligned\n" : "[FAIL] not page aligned\n");

  int zero = 1;
  for (int i = 0; i < 4096; ++i)
    zero = zero && a[i] == 0;
  printf(zero ? "[PASS] new page is zeroed\n" : "[FAIL] new page contains old data\n");
  for (int i = 0; i < 4096; ++i)
    b[i] = 'b';

  printf(freepage(b) == 0 ? "[PASS] freepage succeeded\n" : "[FAIL] freepage failed\n");
  printf(freepage(b) == -1 ? "[PASS] double free rejected\n" : "[FAIL] double free accepted\n");
  printf(freepage(a + 1) == -1 ? "[PASS] unaligned address rejected\n" : "[FAIL] unaligned address accepted\n");
  printf(freepage((void*) 0x8000000) == -1 ? "[PASS] address outside the region rejected\n"
                                           : "[FAIL] code page could be freed\n");

  char* again = allocpage();
  printf(again == b ? "[PASS] lowest free slot reused\n" : "[FAIL] slot not reused\n");
  printf(again[0] == 0 ? "[PASS] reused page is zeroed\n" : "[FAIL] reused page has old content\n");

  int count = 3;
  while (allocpage())
    ++count;
  printf(count == 512 ? "[PASS] region holds exactly 512 pages\n" : "[FAIL] wrong region size\n");

  freepage(c);
  printf("[INFO] touching a freed page - the kernel must kill this process\n");
  c[0] = 1;
  printf("[FAIL] freed page still usable\n");
  return 0;
}
