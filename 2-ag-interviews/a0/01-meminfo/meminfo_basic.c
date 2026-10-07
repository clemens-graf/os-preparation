#include <stdio.h>
#include <nonstd.h>

char untouched[3 * 4096];

int main()
{
  struct meminfo before;
  struct meminfo after;
  if (meminfo(&before) != 0)
  {
    printf("[FAIL] meminfo returned an error\n");
    return 1;
  }
  printf("[INFO] total %zu, free %zu, process %zu\n", before.total_pages, before.free_pages, before.process_pages);
  printf(before.total_pages > before.free_pages && before.free_pages > 0 ? "[PASS] plausible page counts\n"
                                                                         : "[FAIL] implausible page counts\n");
  // pages are loaded lazily: at the start only the code page and the stack page are mapped
  printf(before.process_pages >= 2 ? "[PASS] code and stack are counted\n" : "[FAIL] too few process pages\n");

  char* page = (char*) (((size_t) untouched + 4095) & ~4095UL);
  meminfo(&before);                                       // no printf in between: it would load more pages
  page[0] = 1;                                            // loads exactly one more page
  meminfo(&after);
  printf(after.process_pages == before.process_pages + 1 ? "[PASS] touching a page adds exactly one\n"
                                                         : "[FAIL] process_pages did not grow by one\n");
  printf(after.free_pages < before.free_pages ? "[PASS] free pages went down\n" : "[FAIL] free pages did not change\n");

  printf(meminfo(0) == -1 ? "[PASS] NULL rejected\n" : "[FAIL] NULL accepted\n");
  printf(meminfo((struct meminfo*) 0xFFFFFFFF80000000ULL) == -1 ? "[PASS] kernel address rejected\n"
                                                                : "[FAIL] kernel address accepted\n");
  printf(meminfo((struct meminfo*) 0x7FFFFFFFFFF8ULL) == -1 ? "[PASS] struct crossing USER_BREAK rejected\n"
                                                            : "[FAIL] struct crossing USER_BREAK accepted\n");
  return 0;
}
