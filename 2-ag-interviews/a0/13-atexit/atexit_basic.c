#include <stdio.h>
#include <stdlib.h>

int step = 0;

void first(void)
{
  printf(step == 2 ? "[PASS] first registered runs last\n" : "[FAIL] first ran at step %d\n", step);
  step++;
}

void second(void)
{
  printf(step == 1 ? "[PASS] second registered runs second\n" : "[FAIL] second ran at step %d\n", step);
  step++;
}

void third(void)
{
  printf(step == 0 ? "[PASS] last registered runs first\n" : "[FAIL] third ran at step %d\n", step);
  step++;
}

int main()
{
  printf(atexit(first) == 0 && atexit(second) == 0 && atexit(third) == 0 ? "[PASS] three functions registered\n"
                                                                         : "[FAIL] atexit failed\n");
  printf(atexit(0) == -1 ? "[PASS] NULL rejected\n" : "[FAIL] NULL accepted\n");
  printf("[INFO] returning from main - _start calls exit()\n");
  return 0;
}
