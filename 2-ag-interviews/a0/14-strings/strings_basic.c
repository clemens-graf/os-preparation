#include <stdio.h>
#include <string.h>

int main()
{
  char buffer[32];
  printf(strcpy(buffer, "hello") == buffer && strcmp(buffer, "hello") == 0 ? "[PASS] strcpy copies and returns dest\n"
                                                                           : "[FAIL] strcpy\n");
  printf(strcat(buffer, ", sweb") == buffer && strcmp(buffer, "hello, sweb") == 0 ? "[PASS] strcat appends\n"
                                                                                  : "[FAIL] strcat\n");

  char small[8];
  memset(small, 'x', sizeof(small));
  strncpy(small, "abc", sizeof(small));
  int padded = small[3] == 0 && small[7] == 0;
  printf(strcmp(small, "abc") == 0 && padded ? "[PASS] strncpy pads the rest with 0\n" : "[FAIL] strncpy padding\n");

  memset(small, 'x', sizeof(small));
  strncpy(small, "abcdefghijk", 4);
  printf(small[0] == 'a' && small[3] == 'd' && small[4] == 'x' ? "[PASS] strncpy writes at most n bytes (no NUL!)\n"
                                                               : "[FAIL] strncpy wrote too much\n");

  printf(strncmp("abcdef", "abcxyz", 3) == 0 && strncmp("abcdef", "abcxyz", 4) < 0 && strncmp("b", "a", 5) > 0
         ? "[PASS] strncmp compares at most n bytes\n" : "[FAIL] strncmp\n");
  printf(strncmp("abc", "abc", 100) == 0 ? "[PASS] strncmp stops at the end of equal strings\n"
                                         : "[FAIL] strncmp read past the end\n");

  const char* text = "a/b/c";
  printf(strchr(text, '/') == text + 1 ? "[PASS] strchr finds the first one\n" : "[FAIL] strchr\n");
  printf(strchr(text, 'z') == 0 ? "[PASS] strchr returns NULL if not found\n" : "[FAIL] strchr found nothing\n");
  printf(strchr(text, 0) == text + 5 ? "[PASS] strchr finds the terminating NUL\n" : "[FAIL] strchr and NUL\n");
  return 0;
}
