#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <nonstd.h>

volatile int stop = 0;

size_t now_ns(clockid_t clock)
{
  struct timespec tp;
  clock_gettime(clock, &tp);
  return (size_t) tp.tv_sec * 1000000000 + (size_t) tp.tv_nsec;
}

void busy_ms(size_t ms)                     // burn CPU, but let the others run too
{
  size_t end = now_ns(CLOCK_MONOTONIC) + ms * 1000000;
  while (now_ns(CLOCK_MONOTONIC) < end)
    sched_yield();
}

void* spinner(void* arg)
{
  (void) arg;
  while (!stop)
    for (volatile int i = 0; i < 10000; ++i) {}
  return 0;
}

void* waiter(void* arg)
{
  pthread_join((pthread_t) arg, 0);          // blocked for the whole test
  return 0;
}

int main()
{
  pthread_t spin, wait;
  pthread_create(&spin, 0, spinner, 0);
  pthread_create(&wait, 0, waiter, (void*) spin);
  busy_ms(200);

  ssize_t s1 = thread_cputime(spin);
  busy_ms(300);
  ssize_t s2 = thread_cputime(spin);
  printf(s1 > 0 ? "[PASS] another thread's CPU time is known\n" : "[FAIL] no CPU time for the other thread\n");
  printf(s2 > s1 ? "[PASS] it grows while the thread runs\n" : "[FAIL] it does not grow\n");

  int grows = 0;                             // while WE run (10 ms, no yield), the waiter's time must stay
  for (int k = 0; k < 5; ++k)
  {
    ssize_t w1 = thread_cputime(wait);
    size_t end = now_ns(CLOCK_MONOTONIC) + 10000000;
    while (now_ns(CLOCK_MONOTONIC) < end) {}
    ssize_t w2 = thread_cputime(wait);
    if (w1 < 0 || w2 - w1 > 1000000)
      grows = 1;
  }
  printf(!grows ? "[PASS] a blocked thread's time does not grow\n" : "[FAIL] a blocked thread's time grows\n");

  size_t own = now_ns(CLOCK_THREAD_CPUTIME_ID);
  ssize_t mine = thread_cputime(pthread_self());
  printf(mine >= (ssize_t) own ? "[PASS] own time matches CLOCK_THREAD_CPUTIME_ID\n" : "[FAIL] own time too small\n");
  printf(thread_cputime(9999) == -1 ? "[PASS] unknown thread rejected\n" : "[FAIL] unknown thread accepted\n");

  size_t i1 = timer_irq_time();
  busy_ms(300);
  size_t i2 = timer_irq_time();
  printf(i2 > i1 ? "[PASS] time in the timer interrupt grows\n" : "[FAIL] interrupt time does not grow\n");
  printf(i2 < now_ns(CLOCK_MONOTONIC) ? "[PASS] interrupt time is less than the total time\n"
                                      : "[FAIL] interrupt time too large\n");

  stop = 1;
  pthread_join(wait, 0);
  printf(thread_cputime(spin) == -1 ? "[PASS] an ended thread is rejected\n" : "[FAIL] ended thread still has a time\n");
  return 0;
}
