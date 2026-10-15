#include <stdio.h>
#include <pthread.h>
#include <sched.h>

volatile int quick_done = 0;
volatile int stop = 0;

void* runner(void* arg)
{
  (void) arg;
  while (!stop)
    sched_yield();
  return (void*) 5;
}

void* quick(void* arg)
{
  (void) arg;
  quick_done = 1;
  return 0;
}

void* nothing(void* arg)
{
  return arg;
}

int main()
{
  pthread_t t;
  pthread_create(&t, 0, runner, 0);
  printf(pthread_detach(t) == 0 ? "[PASS] detach a running thread\n" : "[FAIL] detach failed\n");
  printf(pthread_join(t, 0) == -1 ? "[PASS] a detached thread cannot be joined\n" : "[FAIL] joined a detached thread\n");
  printf(pthread_detach(t) == -1 ? "[PASS] detaching twice fails\n" : "[FAIL] detached twice\n");
  stop = 1;

  pthread_t q;
  pthread_create(&q, 0, quick, 0);
  while (!quick_done)
    sched_yield();
  for (int i = 0; i < 5; ++i)
    sched_yield();                              // let it really end
  printf(pthread_detach(q) == 0 && pthread_join(q, 0) == -1 ? "[PASS] detach a thread that has already ended\n"
                                                            : "[FAIL] detach after the end\n");

  printf(pthread_detach(12345) == -1 ? "[PASS] unknown thread rejected\n" : "[FAIL] unknown thread\n");

  int ok = 1;
  for (int i = 0; i < 200 && ok; ++i)
  {
    pthread_t d;
    ok = pthread_create(&d, 0, nothing, 0) == 0 && pthread_detach(d) == 0;
    if (i % 20 == 0)
      sched_yield();
  }
  printf(ok ? "[PASS] 200 detached threads\n" : "[FAIL] create/detach cycles failed\n");
  for (int i = 0; i < 20; ++i)
    sched_yield();
  return 0;
}
