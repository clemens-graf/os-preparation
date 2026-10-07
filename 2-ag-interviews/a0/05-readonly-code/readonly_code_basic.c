#include <stdio.h>
#include <string.h>

int counter = 1;                     // .data: writable
int zeroes[1000];                    // .bss: writable, not in the file
const char message[] = "constant";   // .rodata: read only

int main()
{
  counter++;
  zeroes[999] = 5;
  printf(counter == 2 && zeroes[999] == 5 ? "[PASS] data and bss are writable\n" : "[FAIL] data/bss broken\n");
  printf(strcmp(message, "constant") == 0 ? "[PASS] rodata readable\n" : "[FAIL] rodata unreadable\n");
  unsigned char first = *(unsigned char*) main;
  printf(first != 0 ? "[PASS] code readable\n" : "[FAIL] code unreadable\n");

  printf("[INFO] overwriting the code of main - the kernel must kill this process\n");
  *(unsigned char*) main = 0xC3;      // 'ret'
  printf("[FAIL] code is writable\n");
  return 0;
}
