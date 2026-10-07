#include <stdio.h>
#include <pthread.h>
#include <sched.h>

volatile size_t log[16];
volatile size_t log_count = 0;
volatile size_t stack_region[16];

void* record(void* arg)
{
  int local;
  stack_region[log_count] = (size_t) &local >> 18;   // 256 KiB stack slot of the running thread
  log[log_count] = (size_t) arg;
  log_count++;
  return 0;
}

void* times_ten(void* arg)
{
  return record((void*) ((size_t) arg * 10));
}

int main()
{
  void* (*functions[4])(void*) = {record, times_ten, record, times_ten};
  void* arguments[4] = {(void*) 1, (void*) 2, (void*) 3, (void*) 4};
  pthread_t id = 0;
  printf(pthread_multi(&id, functions, arguments, 4) == 0 ? "[PASS] pthread_multi returned 0\n"
                                                         : "[FAIL] pthread_multi failed\n");
  for (int i = 0; i < 4; ++i)          // the kernel must have copied the arrays already
  {
    functions[i] = 0;
    arguments[i] = (void*) 999;
  }
  while (log_count < 4)
    sched_yield();
  printf("[INFO] log: %zu %zu %zu %zu\n", log[0], log[1], log[2], log[3]);
  printf(log[0] == 1 && log[1] == 20 && log[2] == 3 && log[3] == 40 ? "[PASS] functions ran in order with their arguments\n"
                                                                    : "[FAIL] wrong order or arguments\n");
  printf(stack_region[0] == stack_region[3] && stack_region[0] != ((size_t) &id >> 18)
         ? "[PASS] all ran in one thread, not in main\n" : "[FAIL] not one separate thread\n");
  printf(id != 0 ? "[PASS] thread id written\n" : "[FAIL] no thread id\n");

  void* (*one[1])(void*) = {record};
  void* arg[1] = {0};
  printf(pthread_multi(&id, one, arg, 0) == -1 ? "[PASS] count 0 rejected\n" : "[FAIL] count 0 accepted\n");
  printf(pthread_multi(&id, one, arg, 17) == -1 ? "[PASS] count 17 rejected\n" : "[FAIL] count 17 accepted\n");
  one[0] = (void* (*)(void*)) 0xFFFFFFFF80000000ULL;
  printf(pthread_multi(&id, one, arg, 1) == -1 ? "[PASS] kernel function pointer rejected\n"
                                               : "[FAIL] kernel function pointer accepted\n");
  return 0;
}
