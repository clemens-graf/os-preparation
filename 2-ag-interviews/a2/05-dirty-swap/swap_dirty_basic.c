#include <stdio.h>
#include <nonstd.h>

char data[3 * 4096];

int main()
{
  char* page = (char*) (((size_t) data + 4095) & ~4095UL);
  page[0] = 'x';
  size_t w0 = swapwrites();
  size_t s0 = swapinfo();
  swapout(page);
  printf(swapwrites() == w0 + 1 ? "[PASS] first swap-out writes the page\n" : "[FAIL] first swap-out\n");

  char c = page[0];                       // swap in - the page is clean now
  swapout(page);
  printf(swapwrites() == w0 + 1 ? "[PASS] unchanged page is not written again\n" : "[FAIL] clean page written again\n");
  printf(page[0] == 'x' && c == 'x' ? "[PASS] content still correct\n" : "[FAIL] content wrong\n");

  page[1] = 'y';                          // swap in and write: dirty
  swapout(page);
  printf(swapwrites() == w0 + 2 ? "[PASS] changed page is written\n" : "[FAIL] dirty page not written\n");
  printf(swapinfo() == s0 + 1 ? "[PASS] it reuses its slot\n" : "[FAIL] extra slot\n");
  printf(page[0] == 'x' && page[1] == 'y' ? "[PASS] new content came back\n" : "[FAIL] lost the change\n");
  return 0;                               // exits with a present page that owns a slot
}
