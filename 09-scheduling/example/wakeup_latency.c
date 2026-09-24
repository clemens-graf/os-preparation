/*
 * wakeup_latency.c - why an interactive thread stays responsive.
 *
 * A thread sleeps 1 ms, wakes up and measures how late it woke - 500
 * times - first alone, then competing with 3 CPU-hungry threads on the
 * SAME CPU. The sleeper hardly uses CPU time, so the scheduler lets it
 * run soon after waking (Linux: its virtual runtime is small; an MLFQ
 * keeps such jobs on a high level). The hogs pay with a few context
 * switches; the sleeper's latency grows only a little.
 */
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static volatile int stop;

static void pin_to_cpu0(void)
{
  cpu_set_t one;
  CPU_ZERO(&one);
  CPU_SET(0, &one);
  pthread_setaffinity_np(pthread_self(), sizeof one, &one);
}

static void *hog(void *arg)
{
  pin_to_cpu0();
  volatile unsigned long x = 0;
  while (!stop)
    x++;
  return NULL;
}

static long long ns(struct timespec t)
{
  return t.tv_sec * 1000000000LL + t.tv_nsec;
}

static int cmp(const void *a, const void *b)
{
  long long x = *(const long long *)a, y = *(const long long *)b;
  return (x > y) - (x < y);
}

static void measure(const char *label)
{
  static long long late[500];
  for (int i = 0; i < 500; i++) {
    struct timespec before, after, d = {0, 1000000};
    clock_gettime(CLOCK_MONOTONIC, &before);
    nanosleep(&d, NULL);
    clock_gettime(CLOCK_MONOTONIC, &after);
    late[i] = ns(after) - ns(before) - 1000000;
  }
  qsort(late, 500, sizeof late[0], cmp);
  printf("  %-22s wake-up delay: median %6.1f us, 99th percentile %7.1f us\n", label,
         late[250] / 1e3, late[495] / 1e3);
}

int main(void)
{
  pin_to_cpu0();
  measure("alone on CPU 0");
  pthread_t t[3];
  for (int i = 0; i < 3; i++)
    pthread_create(&t[i], NULL, hog, NULL);
  measure("with 3 CPU hogs");
  stop = 1;
  for (int i = 0; i < 3; i++)
    pthread_join(t[i], NULL);
  return 0;
}
