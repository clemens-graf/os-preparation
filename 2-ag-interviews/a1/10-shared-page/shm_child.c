#include <stdio.h>
#include <string.h>
#include <nonstd.h>

// started by shm_basic.sweb
int main()
{
  char* page = shm_attach(7);
  if (page[100] == 0)
  {
    printf(strcmp(page, "hello from shm_basic") == 0 ? "[PASS] child sees the parent's text\n"
                                                     : "[FAIL] child sees no text\n");
    memcpy(page + 100, "hello back from shm_child", 26);
    shm_detach(7);
  }
  else
  {
    memcpy(page + 200, "second child was here", 22);   // exit without shm_detach
  }
  return 0;
}
