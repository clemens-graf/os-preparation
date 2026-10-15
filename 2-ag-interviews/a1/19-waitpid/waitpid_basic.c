#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sched.h>

int main()
{
  pid_t child = fork();
  if (child == 0)
    exit(42);                                        // child: ends at once
  int status = -1;
  printf(waitpid(child, &status, 0) == child ? "[PASS] waitpid returns the child's pid\n"
                                             : "[FAIL] waitpid failed\n");
  printf(status == 42 ? "[PASS] the exit code arrives\n" : "[FAIL] status %d\n", status);
  printf(waitpid(child, &status, 0) == -1 ? "[PASS] a child can be waited for only once\n"
                                          : "[FAIL] waited twice\n");

  child = fork();
  if (child == 0)
  {
    for (int i = 0; i < 30; ++i)                     // the parent reaches waitpid long before
      sched_yield();
    exit(7);
  }
  status = -1;
  printf(waitpid(child, &status, 0) == child && status == 7 ? "[PASS] waitpid waits for a running child\n"
                                                             : "[FAIL] did not wait\n");

  child = fork();
  if (child == 0)
  {
    *(volatile int*) 0 = 1;                          // killed by the page fault handler
    exit(1);
  }
  status = -1;
  waitpid(child, &status, 0);
  printf(status == 9999 ? "[PASS] a killed child reports its exit code (9999)\n"
                        : "[FAIL] killed child: status %d\n", status);

  printf(waitpid(12345, &status, 0) == -1 ? "[PASS] unknown pid rejected\n" : "[FAIL] unknown pid\n");
  printf(waitpid(getpid(), &status, 0) == -1 ? "[PASS] not my child: rejected\n" : "[FAIL] waited for itself\n");
  return 0;
}
