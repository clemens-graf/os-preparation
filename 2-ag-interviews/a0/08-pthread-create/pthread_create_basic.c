#include <stdio.h>
#include <pthread.h>
#include <sched.h>

volatile size_t result = 0;
volatile int done = 0;
volatile size_t thread_stack = 0;

void* worker(void* arg)
{
  int local = 0;
  thread_stack = (size_t) &local;
  result = (size_t) arg * 2;
  done = 1;
  return 0;
}

volatile int counters[10];
volatile int finished = 0;

void* counting(void* arg)
{
  size_t i = (size_t) arg;
  for (int k = 0; k < 1000; ++k)
    counters[i]++;
  __atomic_add_fetch(&finished, 1, __ATOMIC_SEQ_CST);
  return 0;
}

int main()
{
  int main_local = 0;
  pthread_t id = 0;
  printf(pthread_create(&id, 0, worker, (void*) 21) == 0 ? "[PASS] pthread_create returned 0\n"
                                                         : "[FAIL] pthread_create failed\n");
  while (!done)
    sched_yield();
  printf(result == 42 ? "[PASS] thread ran with its argument\n" : "[FAIL] wrong result\n");
  printf(id != 0 ? "[PASS] thread id written\n" : "[FAIL] no thread id\n");
  printf(thread_stack != 0 && thread_stack != (size_t) &main_local ? "[PASS] thread has its own stack\n"
                                                                   : "[FAIL] same stack\n");
  printf("[INFO] main stack %p, thread stack %zx\n", &main_local, thread_stack);

  pthread_t ids[10];
  for (size_t i = 0; i < 10; ++i)
    pthread_create(&ids[i], 0, counting, (void*) i);
  while (finished < 10)
    sched_yield();
  int all = 1;
  for (int i = 0; i < 10; ++i)
    all = all && counters[i] == 1000;
  printf(all ? "[PASS] 10 threads each counted to 1000\n" : "[FAIL] counters wrong\n");
  int distinct = 1;
  for (int i = 0; i < 10; ++i)
    for (int j = i + 1; j < 10; ++j)
      distinct = distinct && ids[i] != ids[j];
  printf(distinct ? "[PASS] thread ids are distinct\n" : "[FAIL] duplicate thread ids\n");

  printf(pthread_create(&id, 0, (void* (*)(void*)) 0xFFFFFFFF80000000ULL, 0) == -1 ? "[PASS] kernel address rejected\n"
                                                                                  : "[FAIL] kernel address accepted\n");
  printf(pthread_create((pthread_t*) 0xFFFFFFFF80000000ULL, 0, worker, 0) == -1 ? "[PASS] bad id pointer rejected\n"
                                                                               : "[FAIL] bad id pointer accepted\n");
  return 0;
}
