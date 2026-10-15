#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
  char* a = malloc(100);
  char* b = malloc(100);
  printf(a && b && a != b ? "[PASS] two blocks\n" : "[FAIL] malloc returned NULL or the same block\n");
  printf(((size_t) a % 16) == 0 && ((size_t) b % 16) == 0 ? "[PASS] 16-byte aligned\n"
                                                            : "[FAIL] not 16-byte aligned\n");

  memset(a, 'a', 100);
  memset(b, 'b', 100);
  int ok = 1;
  for (int i = 0; i < 100; ++i)
    ok = ok && a[i] == 'a' && b[i] == 'b';
  printf(ok ? "[PASS] blocks do not overlap\n" : "[FAIL] writing one block changed the other\n");

  free(a);
  char* c = malloc(80);                         // fits into a's freed block
  printf(c == a ? "[PASS] freed block is reused\n" : "[FAIL] freed block not reused\n");

  memset(c, 0x55, 80);
  free(c);
  int* z = calloc(20, sizeof(int));             // reuses the dirty block: must be zeroed
  ok = z != 0;
  for (int i = 0; ok && i < 20; ++i)
    ok = z[i] == 0;
  printf(ok ? "[PASS] calloc memory is zeroed\n" : "[FAIL] calloc memory not zeroed\n");

  char* r = malloc(10);
  for (int i = 0; i < 10; ++i)
    r[i] = (char) ('0' + i);
  r = realloc(r, 5000);                         // grows: must keep the old content
  ok = r != 0;
  for (int i = 0; ok && i < 10; ++i)
    ok = r[i] == (char) ('0' + i);
  printf(ok ? "[PASS] realloc keeps the content\n" : "[FAIL] realloc lost the content\n");

  printf(malloc(64 * 1024 * 1024) == 0 ? "[PASS] too large returns NULL\n" : "[FAIL] 64 MiB granted\n");
  free(0);                                      // must not crash
  printf("[PASS] free(NULL) is harmless\n");

  int count = 0;
  char* previous = 0;
  for (int i = 0; i < 1000; ++i)
  {
    char* p = malloc(64);
    if (p == 0 || p == previous)
      break;
    p[0] = (char) i;
    previous = p;
    ++count;
  }
  printf(count == 1000 ? "[PASS] 1000 small blocks\n" : "[FAIL] only %d small blocks\n", count);
  return 0;
}
