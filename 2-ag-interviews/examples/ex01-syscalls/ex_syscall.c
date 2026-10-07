#include <stdio.h>
#include <nonstd.h>
#include <sched.h>

int main()
{
  size_t t0 = getticks();
  for (int i = 0; i < 50; ++i)
    sched_yield();
  size_t t1 = getticks();
  printf("[INFO] ticks %zu -> %zu\n", t0, t1);
  printf(t1 >= t0 ? "[PASS] getticks is monotonic\n" : "[FAIL] getticks went backwards\n");

  char name[64];
  ssize_t length = getthreadname(name, sizeof(name));
  printf("[INFO] name '%s' (length %zd)\n", name, length);
  printf(length > 0 ? "[PASS] getthreadname returned a name\n" : "[FAIL] getthreadname failed\n");

  char tiny[4];
  length = getthreadname(tiny, sizeof(tiny));
  printf(length == 3 && tiny[3] == 0 ? "[PASS] truncated to buffer size\n" : "[FAIL] no truncation\n");

  length = getthreadname((char*) 0xFFFFFFFF80000000ULL, 16); // a kernel address
  printf(length == -1 ? "[PASS] kernel pointer rejected\n" : "[FAIL] kernel pointer accepted!\n");
  return 0;
}
