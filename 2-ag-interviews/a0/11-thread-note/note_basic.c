// A0-11 - a note per thread: the tutorial 1 syscall done right.
#include <stdio.h>
#include <string.h>
#include <nonstd.h>

int main()
{
  char buf[32];
  memset(buf, 'x', sizeof(buf));
  printf(getnote(buf, sizeof(buf)) == 0 && buf[0] == '\0' ? "[PASS] a new thread has an empty note\n"
                                                          : "[FAIL] the first note is not empty\n");

  printf(setnote("hello") == 5 ? "[PASS] setnote(\"hello\"): 5\n" : "[FAIL] setnote did not return 5\n");
  printf(getnote(buf, sizeof(buf)) == 5 && strcmp(buf, "hello") == 0 ? "[PASS] getnote gives it back\n"
                                                                     : "[FAIL] getnote returned something else\n");

  char local[] = "on the stack";
  printf(setnote(local) == 12 ? "[PASS] a string on the stack: 12\n" : "[FAIL] stack string\n");
  local[0] = 'X';                    // the kernel must have its own copy
  printf(getnote(buf, sizeof(buf)) == 12 && strcmp(buf, "on the stack") == 0
             ? "[PASS] changing the user string afterwards does not change the note\n"
             : "[FAIL] the note follows the user string - no kernel copy?\n");

  printf(setnote("0123456789abcdefXYZ") == 15 ? "[PASS] a longer string is cut: 15\n" : "[FAIL] not cut at 15\n");
  printf(getnote(buf, sizeof(buf)) == 15 && strcmp(buf, "0123456789abcde") == 0
             ? "[PASS] the cut note is NUL-terminated\n"
             : "[FAIL] the cut note is wrong (missing NUL?)\n");
  printf(getnote(buf, 4) == 15 && strcmp(buf, "012") == 0
             ? "[PASS] a small buffer: 3 characters + NUL, the full length returned\n"
             : "[FAIL] small buffer handled wrongly\n");

  printf(setnote("") == 0 && getnote(buf, sizeof(buf)) == 0 && buf[0] == '\0' ? "[PASS] the empty string\n"
                                                                             : "[FAIL] empty string\n");

  setnote("parent");
  printf("[INFO] starting note_child.sweb - it sets its own note\n");
  createprocess("/usr/note_child.sweb", 1);
  printf(getnote(buf, sizeof(buf)) == 6 && strcmp(buf, "parent") == 0
             ? "[PASS] the child did not change this thread's note (per thread, not global)\n"
             : "[FAIL] the note changed - is it a global variable?\n");
  return 0;
}
