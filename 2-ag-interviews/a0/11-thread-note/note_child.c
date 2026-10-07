// A0-11 - started by note_basic: another process = another thread = its own note.
#include <stdio.h>
#include <string.h>
#include <nonstd.h>

int main()
{
  char buf[32];
  printf(getnote(buf, sizeof(buf)) == 0 ? "[PASS] child: starts with an empty note\n"
                                        : "[FAIL] child: sees another thread's note\n");
  setnote("child");
  printf(getnote(buf, sizeof(buf)) == 5 && strcmp(buf, "child") == 0 ? "[PASS] child: its own note\n"
                                                                     : "[FAIL] child: note wrong\n");
  return 0;
}
