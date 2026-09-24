/*
 * nice_share.c - how Linux divides ONE CPU between CPU-bound processes.
 *
 * All children are pinned to CPU 0 and spin for 2 seconds; each counts how
 * much CPU time it got. Linux's scheduler (CFS/EEVDF) gives each runnable
 * task a share proportional to its WEIGHT, and the nice value sets the
 * weight: nice 0 = 1024, each nice step is about 1.25x less
 * (nice 5 = 335, nice 10 = 110). Not a fixed priority: a nice-10 process
 * still runs, just less - no starvation.
 */
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static double cpu_seconds(void)
{
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  return ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1e6 + ru.ru_stime.tv_sec +
         ru.ru_stime.tv_usec / 1e6;
}

int main(void)
{
  const int nices[] = {0, 0, 5, 10};
  const int n = sizeof nices / sizeof nices[0];
  int pipes[4][2];

  printf("%d CPU-bound processes on CPU 0 for 2 s:\n", n);
  for (int i = 0; i < n; i++) {
    if (pipe(pipes[i]) != 0) {
      perror("pipe");
      return 1;
    }
    if (fork() == 0) {
      cpu_set_t one;
      CPU_ZERO(&one);
      CPU_SET(0, &one);
      if (sched_setaffinity(0, sizeof one, &one) != 0)
        perror("sched_setaffinity");
      if (setpriority(PRIO_PROCESS, 0, nices[i]) != 0)
        perror("setpriority");
      struct timespec start, now;
      clock_gettime(CLOCK_MONOTONIC, &start);
      do                                    /* spin: always runnable */
        clock_gettime(CLOCK_MONOTONIC, &now);
      while (now.tv_sec - start.tv_sec + (now.tv_nsec - start.tv_nsec) / 1e9 < 2.0);
      double got = cpu_seconds();
      if (write(pipes[i][1], &got, sizeof got) != sizeof got)
        _exit(1);
      _exit(0);
    }
  }
  double total = 0, got[4];
  for (int i = 0; i < n; i++) {
    if (read(pipes[i][0], &got[i], sizeof got[i]) != sizeof got[i])
      got[i] = 0;
    total += got[i];
  }
  while (wait(NULL) > 0) {}
  for (int i = 0; i < n; i++)
    printf("  nice %2d: %.2f s CPU = %4.1f %%\n", nices[i], got[i], 100 * got[i] / total);
  printf("expected by weight: 1024:1024:335:110 -> 41.9 %% 41.9 %% 13.7 %% 4.5 %%\n");
  return 0;
}
