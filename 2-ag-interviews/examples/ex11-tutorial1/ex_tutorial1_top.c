// ex11 - a string that is not terminated before USER_BREAK: the last 4 bytes of user space
// are "abcd" (no NUL). strncpy in the kernel reads on into the address 0x800000000000 -
// a General Protection Fault in kernel mode, the process is killed (exit code 888).
#include <stdio.h>
#include <string.h>
#include <nonstd.h>

int main()
{
  char* top = (char*) (0x800000000000ULL - 4);   // the top slot of the stack page, never used
  memcpy(top, "abcd", 4);
  printf("[INFO] tutorial_syscall(\"abcd\" without NUL, right below USER_BREAK) ...\n");
  int r = tutorial_syscall(top);
  printf("[INFO] returned %d - not reached: the process died in the syscall\n", r);
  return 0;
}
