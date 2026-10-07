#include <stdio.h>

char code[16] = {(char) 0xC3};            // x86 'ret' - in .data

int twice(int x)
{
  return 2 * x;
}

int main()
{
  printf(twice(21) == 42 ? "[PASS] normal code runs\n" : "[FAIL] code broken\n");
  printf("[INFO] calling into .data - the kernel must kill this process (NX)\n");
  ((void (*)(void)) code)();
  printf("[FAIL] executed .data\n");
  return 0;
}
