#include <stdio.h>
#include <pthread.h>
#include <sched.h>

volatile int go = 0;
volatile pthread_t self_of_a = 0, self_of_b = 0, self_of_c = 0;

void* workA(void* arg)
{
  while (!go)
    sched_yield();                 // wait until main has switched the ids
  self_of_a = pthread_self();
  return (void*) 1;
}

void* workB(void* arg)
{
  while (!go)
    sched_yield();
  self_of_b = pthread_self();
  return (void*) 2;
}

void* workC(void* arg)
{
  while (!go)
    sched_yield();
  self_of_c = pthread_self();
  return (void*) 3;
}

int main()
{
  pthread_t a, b;
  pthread_create(&a, 0, workA, 0);
  pthread_create(&b, 0, workB, 0);

  printf(pthread_switch(a, b) == 0 ? "[PASS] pthread_switch(a, b) returns 0\n" : "[FAIL] pthread_switch failed\n");
  go = 1;
  for (int i = 0; i < 20 && (!self_of_a || !self_of_b); ++i)
    sched_yield();
  printf(self_of_a == b ? "[PASS] the thread that ran workA now has id b\n" : "[FAIL] workA's thread did not get id b\n");
  printf(self_of_b == a ? "[PASS] the thread that ran workB now has id a\n" : "[FAIL] workB's thread did not get id a\n");

  void* value = 0;
  pthread_join(a, &value);
  printf(value == (void*) 2 ? "[PASS] join(a) returns workB's result\n" : "[FAIL] join(a) returned the wrong result\n");
  pthread_join(b, &value);
  printf(value == (void*) 1 ? "[PASS] join(b) returns workA's result\n" : "[FAIL] join(b) returned the wrong result\n");

  printf(pthread_switch(a, a) == -1 ? "[PASS] switching a thread with itself fails\n" : "[FAIL] switch(a, a) accepted\n");
  printf(pthread_switch(a, 99999) == -1 ? "[PASS] unknown ids fail\n" : "[FAIL] unknown id accepted\n");

  // the main thread itself takes part
  go = 0;
  pthread_t main_id = pthread_self(), c;
  pthread_create(&c, 0, workC, 0);
  pthread_switch(main_id, c);
  printf(pthread_self() == c ? "[PASS] main's own id changed to c\n" : "[FAIL] main's id did not change\n");
  go = 1;
  pthread_join(main_id, &value);
  printf(value == (void*) 3 ? "[PASS] join(old main id) returns workC's result\n" : "[FAIL] join(old main id) wrong\n");
  return 0;
}
