#include <stdio.h>
#include <pthread.h>
#include <sched.h>

#define THREADS 5

volatile int stop = 0;
volatile int first_runner = -1;

void* loop(void* arg)
{
  int me = (int) (size_t) arg;
  while (!stop)
  {
    __sync_val_compare_and_swap(&first_runner, -1, me);   // only the first one after a reset counts
    sched_yield();
  }
  return 0;
}

int main()
{
  pthread_t t[THREADS];
  for (size_t i = 0; i < THREADS; ++i)
    pthread_create(&t[i], 0, loop, (void*) i);

  int invoke_hits = 0;
  int yield_hits = 0;
  for (int k = 0; k < 20; ++k)
  {
    int target = THREADS - 1 - k % THREADS;
    first_runner = -1;
    pthread_invoke(t[target]);
    invoke_hits += first_runner == target;

    first_runner = -1;
    sched_yield();
    yield_hits += first_runner == target;
  }
  stop = 1;
  for (size_t i = 0; i < THREADS; ++i)
    pthread_join(t[i], 0);
  printf("[INFO] target ran first: %d/20 with pthread_invoke, %d/20 with sched_yield\n", invoke_hits, yield_hits);
  printf(invoke_hits >= 18 ? "[PASS] pthread_invoke runs the target next\n" : "[FAIL] target not run next\n");
  printf(pthread_invoke(12345) == -1 ? "[PASS] unknown thread rejected\n" : "[FAIL] unknown thread accepted\n");
  return 0;
}
