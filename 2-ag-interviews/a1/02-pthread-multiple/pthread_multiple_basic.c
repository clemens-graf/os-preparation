#include <stdio.h>
#include <pthread.h>
#include <sched.h>

volatile int ran = 0;

void* square(void* arg)
{
  __atomic_add_fetch(&ran, 1, __ATOMIC_SEQ_CST);
  return (void*) ((size_t) arg * (size_t) arg);
}

void* work(void* arg)            // gives the CPU away arg times: round robin decides the finishing order
{
  for (size_t i = 0; i < (size_t) arg; ++i)
    sched_yield();
  return arg;
}

int main()
{
  pthread_t ids[64];
  void* args[8];
  for (size_t i = 0; i < 8; ++i)
    args[i] = (void*) (i + 1);

  printf(pthread_multiple(ids, 8, square, args) == 0 ? "[PASS] 8 threads created\n" : "[FAIL] creation failed\n");
  size_t sum = 0;
  void* value;
  for (int i = 0; i < 8; ++i)
  {
    pthread_join(ids[i], &value);
    sum += (size_t) value;
  }
  printf(sum == 204 ? "[PASS] each thread got its own argument\n" : "[FAIL] wrong arguments\n");

  // occupy 60 of the 64 slots with finished but not joined threads
  pthread_t blockers[60];
  pthread_multiple(blockers, 60, square, 0);
  while (ran < 68)
    sched_yield();
  int before = ran;
  printf(pthread_multiple(ids, 8, square, args) == -1 ? "[PASS] 8 threads with only 4 free slots fails\n"
                                                      : "[FAIL] should have failed\n");
  for (int i = 0; i < 10; ++i)
    sched_yield();
  printf(ran == before ? "[PASS] none of the 4 partly created threads ran\n" : "[FAIL] partly created threads ran\n");
  for (int i = 0; i < 60; ++i)
    pthread_join(blockers[i], 0);
  printf(pthread_multiple(ids, 8, square, args) == 0 ? "[PASS] rolled back slots are free again\n"
                                                     : "[FAIL] slots leaked by the rollback\n");
  for (int i = 0; i < 8; ++i)
    pthread_join(ids[i], 0);

  void* durations[3] = {(void*) 6, (void*) 0, (void*) 3};
  pthread_multiple(ids, 3, work, durations);
  size_t index = 99;
  pthread_join_any(ids, 3, &index, &value);
  printf(index == 1 && value == (void*) 0 ? "[PASS] join_any returns the fastest thread first\n"
                                          : "[FAIL] join_any picked the wrong thread\n");
  pthread_join_any(ids, 3, &index, &value);
  printf(index == 2 ? "[PASS] then the next one\n" : "[FAIL] wrong second thread\n");
  pthread_join_any(ids, 3, &index, &value);
  printf(index == 0 ? "[PASS] then the last one\n" : "[FAIL] wrong third thread\n");
  printf(pthread_join_any(ids, 3, &index, &value) == -1 ? "[PASS] all joined: join_any fails instead of waiting\n"
                                                        : "[FAIL] join_any on joined threads\n");
  printf(pthread_multiple(ids, 0, square, 0) == -1 ? "[PASS] count 0 rejected\n" : "[FAIL] count 0 accepted\n");
  return 0;
}
