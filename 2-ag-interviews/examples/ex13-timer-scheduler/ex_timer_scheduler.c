#include <stdio.h>
#include <nonstd.h>
#include <sched.h>
#include <pthread.h>

volatile int stop = 0;
volatile size_t picked_normal = 0, picked_low = 0;

void* worker(void* low)
{
  if (low && lowpriority(1) != 0)
    printf("[FAIL] lowpriority(1) failed\n");
  while (!stop)
    ;                            // busy: only the timer takes the CPU away
  TimesInfo info;
  timesinfo(&info);
  if (low)
    picked_low = info.times_picked;
  else
    picked_normal = info.times_picked;
  return 0;
}

int main()
{
  TimesInfo before, after;
  printf(timesinfo(&before) == 0 ? "[PASS] timesinfo works\n" : "[FAIL] timesinfo failed\n");
  printf(before.times_picked > 0 ? "[PASS] main was picked by the scheduler\n"
                                 : "[FAIL] times_picked is 0\n");

  for (int i = 0; i < 10; ++i)
    sched_yield();               // every yield ends in schedule() - and main is picked again
  timesinfo(&after);
  printf("[INFO] picked %zu -> %zu after 10 yields\n", before.times_picked, after.times_picked);
  printf(after.times_picked >= before.times_picked + 10 ? "[PASS] every yield counted\n"
                                                        : "[FAIL] yields not counted\n");

  for (volatile size_t i = 0; i < 300000000; ++i)
    ;                            // busy for a while: the timer must hit main
  timesinfo(&after);
  printf("[INFO] ticks running %zu -> %zu\n", before.ticks_running, after.ticks_running);
  printf(after.ticks_running > before.ticks_running ? "[PASS] timer ticks counted\n"
                                                    : "[FAIL] no ticks counted\n");

  printf(lowpriority(2) == -1 ? "[PASS] lowpriority(2) rejected\n" : "[FAIL] lowpriority(2) accepted\n");
  printf(timesinfo((TimesInfo*) 0xFFFFFFFF80000000ULL) == -1 ? "[PASS] kernel pointer rejected\n"
                                                              : "[FAIL] kernel pointer accepted\n");

  pthread_t normal, low;
  pthread_create(&normal, 0, worker, (void*) 0);
  pthread_create(&low, 0, worker, (void*) 1);
  for (volatile size_t i = 0; i < 600000000; ++i)
    ;                            // both workers spin meanwhile
  stop = 1;
  pthread_join(normal, 0);
  pthread_join(low, 0);
  printf("[INFO] normal thread picked %zu times, low-priority thread %zu times\n",
         picked_normal, picked_low);
  printf(picked_low > 0 && picked_low * 4 < picked_normal * 3
             ? "[PASS] the low-priority thread ran clearly less often\n"
             : "[FAIL] low priority had no effect\n");
  return 0;
}
