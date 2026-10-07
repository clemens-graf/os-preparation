#include <stdio.h>

// the very first access is a write: the page fault maps the page read-only,
// the write is repeated and must fault again - and kill the process
int main()
{
  printf("[INFO] first access is a write - the kernel must kill this process\n");
  *((char*) 0xC0FFEE000ULL) = 'X';
  printf("[FAIL] magic page is writable\n");
  return 0;
}
