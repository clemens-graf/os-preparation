// ex12 - the tutorial 2 program with checks. Same steps as tutorial2.c (the version from the
// tutorial), but every result is checked and printed as [PASS]/[FAIL]/[INFO].
#include <stdio.h>
#include <nonstd.h>

int global;   // in .bss: its page is mapped by the first page fault that touches it

int main()
{
  int stack = 1;
  global = 2;   // touch it first - an untouched page has no physical page yet

  size_t ppn_global = get_physical_address((size_t) &global);
  size_t ppn_stack = get_physical_address((size_t) &stack);
  printf("[INFO] &global = %p -> ppn %lu (physical address %p)\n", &global, ppn_global,
         (void*) (ppn_global * 4096 + ((size_t) &global & 0xfff)));
  printf("[INFO] &stack  = %p -> ppn %lu\n", &stack, ppn_stack);
  printf(ppn_global != ppn_stack ? "[PASS] global and stack are on different physical pages\n"
                                 : "[FAIL] global and stack on the same physical page?\n");

  // map the page that contains this arbitrary address to the physical page of global
  size_t address = 0x424242424242;
  map_address(address, ppn_global);

  // the alias of global: same page OFFSET, other virtual page number
  size_t vpn = address >> 12;                         // = address / 4096
  size_t offset = (size_t) &global & 0xfff;           // the low 12 bits: same in virtual and physical
  volatile int* alias = (volatile int*) ((vpn << 12) + offset);
  printf("[INFO] vpn of 0x424242424242 = %p, offset of global = %p -> alias at %p\n",
         (void*) vpn, (void*) offset, alias);

  printf(*alias == 2 ? "[PASS] the alias sees global's value\n" : "[FAIL] the alias shows something else\n");
  *alias = 3;
  printf(global == 3 ? "[PASS] a write through the alias changed global\n" : "[FAIL] global unchanged\n");
  global = 4;
  printf(*alias == 4 ? "[PASS] a write to global is visible through the alias\n" : "[FAIL] alias unchanged\n");
  printf(get_physical_address((size_t) alias) == ppn_global ? "[PASS] both addresses translate to the same ppn\n"
                                                             : "[FAIL] different ppns\n");

  printf("[INFO] main returns now. At exit the kernel frees every mapped page of this process -\n"
         "[INFO] ppn %lu is mapped twice, so it is freed twice: KERNEL PANIC 'Double free PPN' (expected)\n",
         ppn_global);
  return 0;
}
