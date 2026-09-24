/*
 * hello.c - run it under strace (`make strace`) and look at how many
 * system calls a trivial program makes: the dynamic loader mapping libc
 * (openat, mmap, mprotect), then a single write for all the printf output
 * (stdio buffering, module 01), then exit_group.
 */
#include <stdio.h>

int main(void)
{
  for (int i = 0; i < 3; i++)
    printf("hello %d\n", i);
  return 0;
}
