// ex11 - calls the tutorial 1 syscall the way a tutor would test it. The kernel's own
// [TUTORIAL] debug lines show what arrived; the [INFO] lines show what came back.
#include <stdio.h>
#include <nonstd.h>

int main()
{
  int r = tutorial_syscall("hello");
  printf(r == 0 ? "[PASS] a normal string: 0\n" : "[FAIL] a normal string failed\n");

  r = tutorial_syscall(0);
  printf("[INFO] NULL -> %d: the kernel returns -1U, but the case drops the result\n", r);
  r = tutorial_syscall((const char*) 0xffffffff80100000);
  printf("[INFO] a kernel address -> %d: refused by the check, yet 0 arrives again\n", r);

  r = tutorial_syscall("this string is much longer than fifteen characters");
  printf("[INFO] a long string -> %d: 15 characters kept, but debug printed all of it\n", r);
  return 0;
}
