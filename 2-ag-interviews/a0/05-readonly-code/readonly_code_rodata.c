#include <stdio.h>

const char message[] = "constant";

int main()
{
  printf("[INFO] writing to a const array - the kernel must kill this process\n");
  ((char*) message)[0] = 'C';
  printf("[FAIL] rodata is writable\n");
  return 0;
}
