#include <stdio.h>
#include <string.h>
#include <nonstd.h>

char data[3 * 4096];

int main()
{
  char* page = (char*) (((size_t) data + 4095) & ~4095UL);
  memset(page, 'A', 4096);
  swapout(page);
  char raw[16];
  printf(swappeek(page, raw) == 0 ? "[PASS] raw slot content readable\n" : "[FAIL] swappeek failed\n");
  int plain = 1;
  for (int i = 0; i < 16; ++i)
    plain = plain && raw[i] == 'A';
  printf(!plain ? "[PASS] the disk does not contain the plain text\n" : "[FAIL] page stored unencrypted\n");
  int ok = 1;
  for (int i = 0; i < 4096; ++i)
    ok = ok && page[i] == 'A';
  printf(ok ? "[PASS] decrypted correctly on swap-in\n" : "[FAIL] content wrong after swap-in\n");
  printf(swappeek(page, raw) == -1 ? "[PASS] a present page has no swap content\n" : "[FAIL] swappeek on present page\n");
  return 0;
}
