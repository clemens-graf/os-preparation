#include <stdio.h>
#include <nonstd.h>

void handler(size_t address)
{
  printf("[FAIL] unregistered handler was called\n");
}

int main()
{
  set_segv_handler(handler);
  set_segv_handler(0);
  printf("[INFO] segfault without handler - the kernel must kill this process normally\n");
  *(volatile int*) 0xDEAD000 = 1;
  printf("[FAIL] survived\n");
  return 0;
}
