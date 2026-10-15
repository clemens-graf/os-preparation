#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handler(void)
{
  printf("[PASS] handler ran on exit() from a nested function\n");
  _exit(0);                                  // _exit must NOT run the handlers again
}

volatile int remaining = 5;

void nested(void)
{
  if (remaining-- > 0)
    nested();
  exit(3);                                   // called 6 frames deep
}

int main()
{
  atexit(handler);
  nested();
  printf("[FAIL] exit returned\n");
  return 1;
}
