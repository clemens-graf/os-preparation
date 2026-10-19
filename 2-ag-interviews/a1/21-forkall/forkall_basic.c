#include <stdio.h>
#include <nonstd.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>

volatile size_t counter = 0;      // the spinner counts while it runs
volatile int stop = 0;            // ends the spinner
volatile size_t joiner_result = 12345;
pthread_t spinner_tid;

void* spinner(void* arg)
{
  while (!stop)
    counter++;                    // pure user mode: copied exactly where it is
  return (void*) 42;
}

void* joiner(void* arg)
{
  joiner_result = pthread_join(spinner_tid, 0);   // waits in the kernel when forkall happens
  return 0;
}

void waitTicks(size_t loops)
{
  for (size_t i = 0; i < loops; ++i)
    sched_yield();
}

int main()
{
  pthread_t joiner_tid;
  pthread_create(&spinner_tid, 0, spinner, 0);
  pthread_create(&joiner_tid, 0, joiner, 0);
  waitTicks(10);                  // both are running / waiting now

  int processes = getProcessCount();
  int pid = forkall();
  if (pid < 0)
  {
    printf("[FAIL] forkall failed\n");
    return 1;
  }

  if (pid == 0)
  {
    // ---- the child ----
    printf("[PASS] the caller's copy returns 0\n");
    size_t before = counter;
    waitTicks(10);                // each yield lets both spinners (parent + child) run a slice
    printf(counter > before ? "[PASS] the spinner was copied: it keeps counting in the child\n"
                            : "[FAIL] the counter stands still: only the caller was copied\n");
    waitTicks(5);
    printf(joiner_result == (size_t) -1 ? "[PASS] the joiner's pthread_join returned -1 in the child\n"
                                        : "[FAIL] the joiner was not copied, or its join did not fail\n");
    stop = 1;                     // the child's own copy of the variable
    void* value = 0;
    printf(pthread_join(spinner_tid, &value) == 0 && value == (void*) 42
               ? "[PASS] the same pthread_t joins the spinner's copy\n"
               : "[FAIL] join of the copied spinner\n");
    exit(0);
  }

  // ---- the parent ----
  printf(pid > 0 ? "[PASS] the parent gets the child's pid\n" : "[FAIL] parent pid\n");
  while (getProcessCount() > processes)   // until the child is gone
    sched_yield();
  printf(joiner_result == 12345 ? "[PASS] the parent's joiner still waits (its join did not fail)\n"
                                : "[FAIL] the parent's joiner was disturbed\n");
  stop = 1;
  pthread_join(joiner_tid, 0);
  printf(joiner_result == 0 ? "[PASS] the parent's threads end normally\n" : "[FAIL] parent's join\n");
  return 0;
}
