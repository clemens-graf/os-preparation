#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <nonstd.h>

volatile size_t counter = 0;
volatile int stop = 0;

void* busy(void* arg)
{
  (void) arg;
  while (!stop)
  {
    counter++;
    sched_yield();
  }
  return 0;
}

int main()
{
  size_t t0 = uptime();
  size_t t1 = uptime();
  printf(t1 >= t0 ? "[PASS] uptime does not go backwards\n" : "[FAIL] uptime went backwards\n");

  t0 = uptime();
  unsigned int left = sleep(1);
  t1 = uptime();
  printf("[INFO] sleep(1) took %zu ticks (18.2 per second)\n", t1 - t0);
  printf(left == 0 ? "[PASS] sleep returns 0\n" : "[FAIL] sleep returned %u\n", left);
  printf(t1 - t0 >= 17 && t1 - t0 <= 30 ? "[PASS] sleep(1) takes about one second\n"
                                        : "[FAIL] sleep(1) took %zu ticks\n", t1 - t0);

  t0 = uptime();
  sleep(0);
  printf(uptime() - t0 <= 2 ? "[PASS] sleep(0) returns at once\n" : "[FAIL] sleep(0) waited\n");

  pthread_t t;
  pthread_create(&t, 0, busy, 0);
  size_t before = counter;
  sleep(1);
  size_t after = counter;
  stop = 1;
  pthread_join(t, 0);
  printf(after > before + 5 ? "[PASS] other threads run while one sleeps\n"
                            : "[FAIL] sleeping blocked the other thread\n");
  return 0;
}
