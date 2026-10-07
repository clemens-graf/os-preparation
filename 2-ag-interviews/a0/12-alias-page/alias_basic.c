// A0-12 - two addresses, one page: the tutorial 2 scenario with checks.
#include <stdio.h>
#include <nonstd.h>

int global;                          // .bss - mapped by the first page fault that touches it
static char untouched[3 * 4096];     // its middle page is never touched

int main()
{
  int stack = 1;
  global = 2;                        // touch it: now its page has a physical page

  size_t ppn = get_physical_address((size_t) &global);
  size_t ppn_stack = get_physical_address((size_t) &stack);
  printf("[INFO] &global = %p -> ppn %lu, &stack = %p -> ppn %lu\n", &global, ppn, &stack, ppn_stack);
  printf(ppn != (size_t) -1 ? "[PASS] global's page has a physical page\n" : "[FAIL] no ppn for global\n");
  printf(ppn_stack != (size_t) -1 && ppn_stack != ppn ? "[PASS] the stack is on another physical page\n"
                                                      : "[FAIL] wrong ppn for the stack\n");
  printf(get_physical_address((size_t) (untouched + 4096)) == (size_t) -1
             ? "[PASS] a page that was never touched: -1\n"
             : "[FAIL] untouched page reported as mapped (0 is a real ppn - use -1)\n");
  printf(get_physical_address(0xffffffff80100000) == (size_t) -1 ? "[PASS] kernel address: -1\n"
                                                                 : "[FAIL] kernel address translated\n");

  // two aliases of global's page: the same offset on two other virtual pages
  size_t offset = (size_t) &global & 0xfff;
  volatile int* alias1 = (volatile int*) (0x424242424000 + offset);
  volatile int* alias2 = (volatile int*) (0x434343434000 + offset);
  printf(map_address(0x424242424242, ppn) == 0 ? "[PASS] map_address with an address inside the page\n"
                                               : "[FAIL] map_address failed\n");
  printf(map_address(0x434343434000, ppn) == 0 ? "[PASS] a second alias of the same page\n"
                                               : "[FAIL] second alias failed\n");

  printf(*alias1 == 2 && *alias2 == 2 ? "[PASS] both aliases see global's value\n" : "[FAIL] aliases differ\n");
  *alias1 = 3;
  printf(global == 3 && *alias2 == 3 ? "[PASS] a write through alias 1 shows everywhere\n"
                                     : "[FAIL] write through alias 1 lost\n");
  global = 4;
  printf(*alias1 == 4 && *alias2 == 4 ? "[PASS] a write to global shows in both aliases\n"
                                      : "[FAIL] aliases did not see the write\n");
  printf(get_physical_address((size_t) alias1) == ppn && get_physical_address((size_t) alias2) == ppn
             ? "[PASS] all three addresses translate to the same ppn\n"
             : "[FAIL] the aliases have other ppns\n");

  // the stack page works the same way
  volatile int* stack_alias = (volatile int*) (0x444444444000 + ((size_t) &stack & 0xfff));
  printf(map_address(0x444444444000, ppn_stack) == 0 ? "[PASS] alias of the stack page\n"
                                                     : "[FAIL] stack alias failed\n");
  *stack_alias = 42;
  printf(stack == 42 ? "[PASS] a write through the stack alias changed the local variable\n"
                     : "[FAIL] local variable unchanged\n");

  printf("[INFO] exiting: the kernel must free every frame exactly once (no panic, no leak)\n");
  return 0;
}
