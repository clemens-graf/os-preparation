#include <stdio.h>
#include <string.h>
#include <nonstd.h>

int main()
{
  char* page = shm_attach(7);
  printf(page ? "[PASS] attached key 7\n" : "[FAIL] attach failed\n");
  memcpy(page, "hello from shm_basic", 21);
  printf(shm_attach(7) == page ? "[PASS] attaching twice gives the same address\n" : "[FAIL] second attach\n");

  createprocess("/usr/shm_child.sweb", 1);           // runs and waits for it
  printf(strcmp(page + 100, "hello back from shm_child") == 0 ? "[PASS] the child wrote into our page\n"
                                                              : "[FAIL] child's answer missing\n");
  createprocess("/usr/shm_child.sweb", 1);           // second child exits WITHOUT detaching
  printf(strcmp(page + 200, "second child was here") == 0 ? "[PASS] page survives a child that did not detach\n"
                                                          : "[FAIL] page lost after a child exited\n");

  printf(shm_detach(7) == 0 ? "[PASS] detached\n" : "[FAIL] detach failed\n");
  printf(shm_detach(7) == -1 ? "[PASS] detaching twice fails\n" : "[FAIL] detached twice\n");
  page = shm_attach(7);
  printf(page[0] == 0 ? "[PASS] after the last detach the page was freed: a new one is zero\n"
                      : "[FAIL] old page still there\n");
  printf(shm_attach(0) == 0 && shm_attach(16) == 0 ? "[PASS] invalid keys rejected\n" : "[FAIL] invalid keys\n");
  return 0;                                          // exits attached: the kernel must clean up
}
