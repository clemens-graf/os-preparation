#include <stdio.h>
#include <nonstd.h>

void handler(size_t address)
{
  printf(address == 8 ? "[PASS] null pointer access reported at address 8\n" : "[FAIL] wrong address\n");
  printf("[INFO] handler returns - the process must exit with code 139\n");
}

int main()
{
  set_segv_handler(handler);
  printf("%d\n", *(volatile int*) 8);
  printf("[FAIL] returned to main after a segfault\n");
  return 0;
}
