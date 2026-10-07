#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

volatile int stop = 0;
volatile int quit = 0;

void* busy(void* arg)
{
  while (!stop)
    ;                                   // uses the CPU until preempted
  while (!quit)
    sched_yield();
  return 0;
}

void* lazy(void* arg)
{
  while (!quit)
    sched_yield();                      // gives the CPU away immediately
  return 0;
}

int main()
{
  clock_t start = clock();
  pthread_t b, l;
  pthread_create(&b, 0, busy, 0);
  pthread_create(&l, 0, lazy, 0);
  for (int i = 0; i < 20; ++i)
    sched_yield();
  stop = 1;
  ssize_t busy_ticks = thread_runtime(b);
  ssize_t lazy_ticks = thread_runtime(l);
  quit = 1;
  pthread_join(b, 0);
  pthread_join(l, 0);
  printf("[INFO] busy thread %zd ticks, lazy thread %zd ticks\n", busy_ticks, lazy_ticks);
  printf(busy_ticks >= 10 ? "[PASS] the busy thread was charged ticks\n" : "[FAIL] busy thread has too few ticks\n");
  printf(lazy_ticks * 4 <= busy_ticks ? "[PASS] the lazy thread used much less\n" : "[FAIL] lazy thread charged too much\n");
  printf(thread_runtime(b) == -1 ? "[PASS] joined thread no longer exists\n" : "[FAIL] joined thread still found\n");

  clock_t used = clock() - start;
  printf("[INFO] clock(): %u microseconds for the whole process\n", used);
  printf(used >= (clock_t) busy_ticks * 54925 ? "[PASS] clock() includes the finished threads\n"
                                              : "[FAIL] clock() lost the threads' time\n");
  printf(thread_runtime(0) >= 0 ? "[PASS] main thread runtime available\n" : "[FAIL] no main thread runtime\n");
  return 0;
}
