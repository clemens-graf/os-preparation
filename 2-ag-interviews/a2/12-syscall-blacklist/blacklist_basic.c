#include <stdio.h>
#include <sched.h>
#include <nonstd.h>

int main()
{
  printf(sched_yield() == 0 ? "[PASS] sched_yield works before\n" : "[FAIL] sched_yield broken\n");
  printf(blacklist(sc_sched_yield) == 0 ? "[PASS] sched_yield blacklisted\n" : "[FAIL] blacklist failed\n");
  printf(sched_yield() == -1 ? "[PASS] sched_yield now returns -1\n" : "[FAIL] sched_yield still works\n");
  printf(blacklist(sc_sched_yield) == 0 ? "[PASS] blacklisting twice is fine\n" : "[FAIL] second blacklist\n");
  printf(blacklist(sc_exit) == -1 ? "[PASS] exit cannot be blacklisted\n" : "[FAIL] exit blacklisted\n");
  printf("[INFO] blacklisting write - nothing may be printed after this line\n");
  blacklist(sc_write);
  printf("[FAIL] write still works\n");
  return 0;
}
