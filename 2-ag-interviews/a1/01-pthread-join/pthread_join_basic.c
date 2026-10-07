#include <stdio.h>
#include <pthread.h>
#include <sched.h>

void* returnArg(void* arg)
{
  return arg;
}

volatile int slow_done = 0;
void* slow(void* arg)
{
  for (volatile int i = 0; i < 3000000; ++i)   // compute, don't yield: a yield costs up to a timer tick
    ;
  slow_done = 1;
  return (void*) 7;
}

void* exitEarly(void* arg)
{
  pthread_exit((void*) 99);
  return (void*) 1;                       // never reached
}

volatile pthread_t self_id;
volatile int self_result = 0;
void* joinSelf(void* arg)
{
  while (self_id == 0)
    sched_yield();
  self_result = pthread_join(self_id, 0);
  return 0;
}

void* crash(void* arg)
{
  *(volatile int*) 0 = 1;                 // killed by the page fault handler
  return 0;
}

int main()
{
  pthread_t t;
  void* value = 0;

  pthread_create(&t, 0, returnArg, (void*) 42);
  printf(pthread_join(t, &value) == 0 && value == (void*) 42 ? "[PASS] join returns the thread's result\n"
                                                             : "[FAIL] wrong join result\n");
  printf(pthread_join(t, &value) == -1 ? "[PASS] second join of the same thread fails\n"
                                       : "[FAIL] joined twice\n");

  pthread_create(&t, 0, slow, 0);
  value = 0;
  pthread_join(t, &value);
  printf(slow_done == 1 && value == (void*) 7 ? "[PASS] join waits until the thread is done\n"
                                              : "[FAIL] join returned too early\n");

  pthread_create(&t, 0, exitEarly, 0);
  for (int i = 0; i < 5; ++i)
    sched_yield();                         // the thread is gone by now
  pthread_join(t, &value);
  printf(value == (void*) 99 ? "[PASS] pthread_exit value, joined after the thread ended\n"
                             : "[FAIL] pthread_exit value lost\n");

  pthread_create((pthread_t*) &self_id, 0, joinSelf, 0);
  pthread_join(self_id, 0);
  printf(self_result == -1 ? "[PASS] joining yourself fails\n" : "[FAIL] self join did not fail\n");

  printf(pthread_join(12345, 0) == -1 ? "[PASS] unknown thread id fails\n" : "[FAIL] unknown id joined\n");

  int ok = 1;
  for (size_t i = 0; i < 100 && ok; ++i)        // more than 64 slots: slots must be reused
  {
    ok = pthread_create(&t, 0, returnArg, (void*) i) == 0 && pthread_join(t, &value) == 0 && value == (void*) i;
  }
  printf(ok ? "[PASS] 100 create/join cycles (slots reused)\n" : "[FAIL] create/join cycle failed\n");

  pthread_t many[10];
  for (size_t i = 0; i < 10; ++i)
    pthread_create(&many[i], 0, returnArg, (void*) (i + 1));
  size_t sum = 0;
  for (int i = 9; i >= 0; --i)                  // join in reverse order
  {
    pthread_join(many[i], &value);
    sum += (size_t) value;
  }
  printf(sum == 55 ? "[PASS] 10 threads joined in reverse order\n" : "[FAIL] wrong sum\n");

  pthread_create(&t, 0, crash, 0);
  int result = pthread_join(t, &value);
  printf(result == 0 && value == (void*) -1 ? "[PASS] a killed thread can be joined (value -1)\n"
                                            : "[FAIL] killed thread not joinable\n");
  return 0;
}
