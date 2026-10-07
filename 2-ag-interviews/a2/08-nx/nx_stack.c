#include <stdio.h>

int main()
{
  volatile char code[16];
  code[0] = (char) 0xC3;                  // 'ret' on the stack
  printf("[INFO] calling into the stack - the kernel must kill this process (NX)\n");
  ((void (*)(void)) code)();
  printf("[FAIL] executed the stack\n");
  return 0;
}
