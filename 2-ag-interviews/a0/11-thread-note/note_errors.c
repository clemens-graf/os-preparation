// A0-11 - what setnote/getnote must refuse. None of these may kill the process or read or
// write beyond USER_BREAK.
#include <stdio.h>
#include <string.h>
#include <nonstd.h>

int main()
{
  char buf[32];
  setnote("keep");

  printf(setnote(0) == (size_t) -1 ? "[PASS] setnote(NULL): -1\n" : "[FAIL] NULL accepted\n");
  printf(setnote((const char*) 0xffffffff80100000) == (size_t) -1 ? "[PASS] kernel address: -1\n"
                                                                  : "[FAIL] kernel address accepted\n");
  printf(setnote((const char*) 0x800000000000) == (size_t) -1 ? "[PASS] USER_BREAK: -1\n"
                                                              : "[FAIL] USER_BREAK accepted\n");

  // the last 4 bytes of user space (the top slot of the stack page, never used)
  char* top = (char*) (0x800000000000ULL - 4);
  memcpy(top, "abcd", 4);           // no NUL before USER_BREAK
  printf("[INFO] setnote on \"abcd\" without NUL, right below USER_BREAK ...\n");
  printf(setnote(top) == (size_t) -1 ? "[PASS] a string that runs into USER_BREAK: -1 (and the process lives)\n"
                                     : "[FAIL] accepted a string that runs into USER_BREAK\n");
  printf(getnote(buf, sizeof(buf)) == 4 && strcmp(buf, "keep") == 0
             ? "[PASS] the refused calls left the note alone\n"
             : "[FAIL] a refused call changed the note\n");

  memcpy(top, "abc", 4);            // now the NUL is the very last user byte
  printf(setnote(top) == 3 ? "[PASS] a string that ends at the last user byte: 3\n"
                           : "[FAIL] the last user byte is still user space\n");

  printf(getnote(0, 16) == (size_t) -1 ? "[PASS] getnote(NULL): -1\n" : "[FAIL] getnote(NULL) accepted\n");
  printf(getnote((char*) 0xffffffff80100000, 16) == (size_t) -1 ? "[PASS] getnote into the kernel: -1\n"
                                                                 : "[FAIL] getnote wrote to a kernel address\n");
  printf(getnote(buf, 0) == (size_t) -1 ? "[PASS] getnote with size 0: -1\n" : "[FAIL] size 0 accepted\n");
  printf(getnote(top, 5) == (size_t) -1 ? "[PASS] a buffer that crosses USER_BREAK: -1\n"
                                        : "[FAIL] buffer across USER_BREAK accepted\n");
  printf(getnote(top, 4) == 3 && strcmp(top, "abc") == 0 ? "[PASS] a buffer that ends exactly at USER_BREAK\n"
                                                         : "[FAIL] exact-fit buffer refused\n");
  return 0;
}
