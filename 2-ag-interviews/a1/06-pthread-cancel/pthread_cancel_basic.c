#include <stdio.h>
#include <pthread.h>
#include <sched.h>

void* yielder(void* arg)
{
  while (1)
    sched_yield();                         // every syscall is a cancellation point
  return 0;
}

volatile int stop_waitee = 0;
void* waitee(void* arg)
{
  while (!stop_waitee)
    sched_yield();
  return (void*) 5;
}

volatile pthread_t waitee_id;
void* joiner(void* arg)
{
  pthread_join(waitee_id, 0);              // sleeps in the kernel
  return (void*) 1;                        // must never get here
}

volatile int go_on = 0;
volatile int after_syscall = 0;
volatile size_t spins = 0;
void* computer(void* arg)
{
  while (!go_on)
    spins++;                               // no syscall: cannot be cancelled here
  sched_yield();                           // first cancellation point
  after_syscall = 1;                       // must never run
  return 0;
}

void* quick(void* arg)
{
  return 0;
}

int main()
{
  pthread_t t;
  void* value = 0;

  pthread_create(&t, 0, yielder, 0);
  printf(pthread_cancel(t) == 0 ? "[PASS] cancel returned 0\n" : "[FAIL] cancel failed\n");
  pthread_join(t, &value);
  printf(value == PTHREAD_CANCELED ? "[PASS] a cancelled thread joins with PTHREAD_CANCELED\n"
                                   : "[FAIL] wrong value for a cancelled thread\n");

  pthread_create((pthread_t*) &waitee_id, 0, waitee, 0);
  pthread_t j;
  pthread_create(&j, 0, joiner, 0);
  for (int i = 0; i < 5; ++i)
    sched_yield();                         // let the joiner fall asleep in pthread_join
  pthread_cancel(j);
  pthread_join(j, &value);
  printf(value == PTHREAD_CANCELED ? "[PASS] a thread sleeping in pthread_join is cancelled\n"
                                   : "[FAIL] sleeping thread not cancelled\n");
  stop_waitee = 1;
  pthread_join(waitee_id, &value);
  printf(value == (void*) 5 ? "[PASS] the thread it waited for is unaffected\n" : "[FAIL] waitee affected\n");

  pthread_create(&t, 0, computer, 0);
  for (int i = 0; i < 3; ++i)
    sched_yield();
  pthread_cancel(t);
  size_t before = spins;
  for (int i = 0; i < 3; ++i)
    sched_yield();
  printf(spins > before ? "[PASS] without a syscall the thread keeps running (deferred)\n"
                        : "[FAIL] thread stopped without a cancellation point\n");
  go_on = 1;
  pthread_join(t, &value);
  printf(value == PTHREAD_CANCELED && !after_syscall ? "[PASS] cancelled at its next syscall\n"
                                                     : "[FAIL] not cancelled at the syscall\n");

  printf(pthread_cancel(12345) == -1 ? "[PASS] unknown thread rejected\n" : "[FAIL] unknown thread accepted\n");
  pthread_create(&t, 0, quick, 0);
  for (int i = 0; i < 5; ++i)
    sched_yield();
  printf(pthread_cancel(t) == -1 ? "[PASS] finished thread cannot be cancelled\n" : "[FAIL] finished thread cancelled\n");
  pthread_join(t, &value);
  printf(value == 0 ? "[PASS] and joins with its own value\n" : "[FAIL] value changed\n");
  return 0;
}
