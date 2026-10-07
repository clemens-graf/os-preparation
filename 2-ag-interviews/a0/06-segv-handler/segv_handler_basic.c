#include <stdio.h>
#include <nonstd.h>

void handler(size_t address)
{
  volatile int local = 7;                 // the handler has a working stack
  printf("[INFO] handler called for address %zx\n", address);
  printf(address == 0xDEAD000 ? "[PASS] handler got the faulting address\n" : "[FAIL] wrong address\n");
  printf(local == 7 ? "[PASS] handler stack works\n" : "[FAIL] handler stack broken\n");
  printf("[INFO] faulting inside the handler - one-shot, so the kernel must kill this process\n");
  *(volatile int*) 0 = 1;
  printf("[FAIL] survived a second fault\n");
}

int main()
{
  printf(set_segv_handler((void (*)(size_t)) 0xFFFFFFFF80000000ULL) == -1 ? "[PASS] kernel address rejected\n"
                                                                          : "[FAIL] kernel address accepted\n");
  printf(set_segv_handler(handler) == 0 ? "[PASS] handler registered\n" : "[FAIL] registration failed\n");
  *(volatile int*) 0xDEAD000 = 1;         // no segment there
  printf("[FAIL] returned to main after a segfault\n");
  return 0;
}
