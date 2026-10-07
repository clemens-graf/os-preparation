#include <stdio.h>
#include <string.h>

#define MAGIC ((char*) 0xC0FFEE000ULL)

int main()
{
  printf("[INFO] magic page says: '%s'\n", MAGIC);
  printf(strstr(MAGIC, "coffee for ") == MAGIC ? "[PASS] starts with 'coffee for '\n" : "[FAIL] wrong prefix\n");
  printf(strstr(MAGIC, "magic_page_basic.sweb") ? "[PASS] contains the program name\n" : "[FAIL] program name missing\n");
  printf(MAGIC[4095] == 0 && MAGIC[2000] == 0 ? "[PASS] rest of the page is zero\n" : "[FAIL] garbage on the page\n");

  printf("[INFO] writing to the magic page - the kernel must kill this process\n");
  MAGIC[0] = 'X';
  printf("[FAIL] magic page is writable\n");
  return 0;
}
