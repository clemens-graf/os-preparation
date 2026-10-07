#include <stdio.h>
#include <pthread.h>
#include <sched.h>

volatile int stop = 0;
volatile size_t counter[2];

void* spin(void* arg)
{
  size_t i = (size_t) arg;
  while (!stop)
    counter[i]++;                        // no yield: only the timer takes the CPU away
  return 0;
}

// runs both threads for a while, returns counter[0] * 100 / counter[1]
size_t race(int flag_first)
{
  pthread_t t[2];
  stop = 0;
  counter[0] = counter[1] = 0;
  pthread_create(&t[0], 0, spin, (void*) 0);
  pthread_create(&t[1], 0, spin, (void*) 1);
  if (flag_first)
    pthread_setdouble(t[0], 1);
  for (int i = 0; i < 15; ++i)
    sched_yield();
  stop = 1;
  pthread_join(t[0], 0);
  pthread_join(t[1], 0);
  return counter[1] ? counter[0] * 100 / counter[1] : 0;
}

int main()
{
  size_t equal = race(0);
  size_t flagged = race(1);
  printf("[INFO] without flag: thread 0 got %zu%% of thread 1's work, with flag: %zu%%\n", equal, flagged);
  printf(equal > 60 && equal < 160 ? "[PASS] without the flag both get about the same\n"
                                   : "[FAIL] unfair without the flag\n");
  printf(flagged > 160 && flagged < 260 ? "[PASS] the flagged thread gets about twice as much\n"
                                        : "[FAIL] the flag does not double the share\n");
  printf(pthread_setdouble(12345, 1) == -1 ? "[PASS] unknown thread rejected\n" : "[FAIL] unknown thread accepted\n");
  return 0;
}
