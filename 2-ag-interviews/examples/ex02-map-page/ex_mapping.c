#include <stdio.h>
#include <string.h>
#include <nonstd.h>

int main()
{
  char* page = scratchpage();
  printf("[INFO] scratch page at %p: '%s'\n", page, page);
  printf(strcmp(page, "hello from the kernel") == 0 ? "[PASS] kernel wrote the page\n" : "[FAIL] wrong content\n");

  for (int i = 100; i < 4096; ++i)              // the whole page is usable
    page[i] = (char) i;
  char* again = scratchpage();
  int same = again == page;
  for (int i = 100; i < 4096 && same; ++i)
    same = page[i] == (char) i;
  printf(same ? "[PASS] second call keeps the page\n" : "[FAIL] second call changed the page\n");
  return 0;
}
