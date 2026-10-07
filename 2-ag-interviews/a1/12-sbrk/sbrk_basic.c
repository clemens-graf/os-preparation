#include <stdio.h>
#include <unistd.h>

int main()
{
  char* start = sbrk(0);
  printf("[INFO] program break at %p\n", start);
  printf(sbrk(10000) == start ? "[PASS] sbrk returns the old break\n" : "[FAIL] wrong old break\n");
  printf(sbrk(0) == start + 10000 ? "[PASS] break moved by 10000 bytes\n" : "[FAIL] break did not move\n");
  for (int i = 0; i < 10000; ++i)
    start[i] = (char) i;
  int ok = 1;
  for (int i = 0; i < 10000; ++i)
    ok = ok && start[i] == (char) i;
  printf(ok ? "[PASS] the 10000 bytes are usable\n" : "[FAIL] heap content wrong\n");

  // a tiny bump allocator on top of sbrk
  int* numbers = sbrk(100 * sizeof(int));
  for (int i = 0; i < 100; ++i)
    numbers[i] = i * i;
  printf(numbers[99] == 9801 && (char*) numbers == start + 10000 ? "[PASS] second allocation follows the first\n"
                                                                 : "[FAIL] second allocation\n");
  sbrk(-(100 * (intptr_t) sizeof(int)));
  printf(sbrk(-20000) == (void*) -1 ? "[PASS] shrinking below the start fails\n" : "[FAIL] break below start\n");
  printf(sbrk(0) == start + 10000 ? "[PASS] a failed sbrk changes nothing\n" : "[FAIL] failed sbrk moved the break\n");

  sbrk(-10000 + 100);                  // break at start + 100: page 0 stays, pages 1 and 2 go away
  printf(start[50] == 50 ? "[PASS] bytes below the break survive shrinking\n" : "[FAIL] lost data below the break\n");
  printf("[INFO] accessing above the shrunk break - the kernel must kill this process\n");
  start[5000] = 1;
  printf("[FAIL] access above the break worked\n");
  return 0;
}
