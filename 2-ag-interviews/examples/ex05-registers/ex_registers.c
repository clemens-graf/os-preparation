#include <stdio.h>
#include <stdlib.h>
#include <nonstd.h>

void landing(size_t argument)
{
  double d = 1.5;                      // SSE instructions fault on a misaligned stack
  printf(argument == 42 ? "[PASS] landed with argument 42\n" : "[FAIL] wrong argument\n");
  printf(d * 2 == 3.0 ? "[PASS] stack is usable\n" : "[FAIL] stack broken\n");
  exit(0);
}

int main()
{
  printf(jump((void (*)(size_t)) 0xFFFFFFFF80000000ULL, 0) == -1 ? "[PASS] kernel address rejected\n"
                                                                  : "[FAIL] kernel address accepted\n");
  jump(landing, 42);
  printf("[FAIL] jump returned\n");
  return 1;
}
