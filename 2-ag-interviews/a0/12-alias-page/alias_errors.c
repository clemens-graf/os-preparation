// A0-12 - what map_address must refuse. Every bad call returns -1 - none of them may
// crash the kernel or hand out a frame that does not belong to this process.
#include <stdio.h>
#include <nonstd.h>

int global;

int main()
{
  global = 1;
  size_t ppn = get_physical_address((size_t) &global);
  size_t page = 0x454545454000;      // stays unmapped until the last check

  printf(map_address(0x424242424000, ppn) == 0 ? "[PASS] a valid alias (setup)\n" : "[FAIL] valid alias refused\n");
  printf(map_address(0x424242424000, ppn) == (size_t) -1 ? "[PASS] the same address again: -1 (not a panic)\n"
                                                         : "[FAIL] mapped twice\n");
  printf(map_address((size_t) &global, ppn) == (size_t) -1 ? "[PASS] a page of the program itself: -1\n"
                                                           : "[FAIL] remapped a page of the program\n");
  printf(map_address(0x800000000000, ppn) == (size_t) -1 ? "[PASS] USER_BREAK: -1\n" : "[FAIL] USER_BREAK accepted\n");
  printf(map_address(0xffffffff80100000, ppn) == (size_t) -1 ? "[PASS] kernel address: -1\n"
                                                             : "[FAIL] kernel address accepted\n");
  printf(map_address(page, 256) == (size_t) -1 ? "[PASS] a frame of the kernel (ppn 256): -1\n"
                                               : "[FAIL] the process got a kernel frame\n");
  printf(map_address(page, 1) == (size_t) -1 ? "[PASS] ppn 1: -1\n" : "[FAIL] ppn 1 accepted\n");
  printf(map_address(page, 0x7fffffff) == (size_t) -1 ? "[PASS] a ppn beyond the end of memory: -1\n"
                                                      : "[FAIL] ppn beyond memory accepted\n");
  printf(map_address(page, (size_t) -1) == (size_t) -1 ? "[PASS] ppn -1 (from a failed lookup): -1\n"
                                                        : "[FAIL] ppn -1 accepted\n");
  printf(get_physical_address(page) == (size_t) -1 ? "[PASS] the refused calls mapped nothing\n"
                                                   : "[FAIL] a refused call left a mapping\n");
  printf(map_address(page, ppn) == 0 ? "[PASS] the same address with a valid ppn works\n"
                                     : "[FAIL] valid call refused\n");
  return 0;
}
