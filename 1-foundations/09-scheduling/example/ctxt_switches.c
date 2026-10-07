/*
 * ctxt_switches.c - voluntary vs. involuntary context switches.
 *
 * Voluntary: the thread gives up the CPU itself - it blocks (sleep, I/O,
 * a lock, join) or yields. Involuntary: the timer interrupt ends its time
 * slice because someone else wants the CPU (preemption). Linux counts
 * both per task in /proc/<pid>/status. SWEB: Scheduler::yield() is the
 * voluntary path, the timer interrupt calling Scheduler::schedule() the
 * involuntary one.
 *
 * Surprise: Linux books a switch caused by sched_yield() as INvoluntary -
 * the thread stays runnable, it only lets others go first. And a yield
 * with nobody else ready is a no-op.
 */
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

static void counts(const char *label)
{
  char path[64], line[128];
  long vol = -1, invol = -1;
  snprintf(path, sizeof path, "/proc/self/task/%ld/status", (long)syscall(SYS_gettid));
  FILE *f = fopen(path, "r");
  while (f && fgets(line, sizeof line, f)) {
    sscanf(line, "voluntary_ctxt_switches: %ld", &vol);
    sscanf(line, "nonvoluntary_ctxt_switches: %ld", &invol);
  }
  if (f)
    fclose(f);
  printf("  %-38s voluntary %5ld, involuntary %5ld\n", label, vol, invol);
}

static void *sleeper(void *arg)
{
  for (int i = 0; i < 200; i++) {
    struct timespec d = {0, 100000};
    nanosleep(&d, NULL);
  }
  counts("sleeper (200 short sleeps)");
  return NULL;
}

static void *yielder(void *arg)
{
  for (int i = 0; i < 200; i++)
    sched_yield();
  counts("yielder (200 sched_yield)");
  return NULL;
}

static void *spinner(void *arg)
{
  struct timespec start, now;
  clock_gettime(CLOCK_MONOTONIC, &start);
  do
    clock_gettime(CLOCK_MONOTONIC, &now);
  while (now.tv_sec - start.tv_sec < 1);
  counts("spinner (1 s of pure computation)");
  return NULL;
}

int main(void)
{
  /* Everyone on CPU 0, so there is competition. */
  cpu_set_t one;
  CPU_ZERO(&one);
  CPU_SET(0, &one);
  sched_setaffinity(0, sizeof one, &one);

  pthread_t t[4];
  pthread_create(&t[0], NULL, sleeper, NULL);
  pthread_create(&t[1], NULL, yielder, NULL);
  pthread_create(&t[2], NULL, spinner, NULL);
  pthread_create(&t[3], NULL, spinner, NULL);
  for (int i = 0; i < 4; i++)
    pthread_join(t[i], NULL);
  return 0;
}
