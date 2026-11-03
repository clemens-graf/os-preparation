#include <stdio.h>
#include <pthread.h>
#include <sched.h>

volatile int stop = 0;
volatile int calls = 0;
volatile pthread_t ran_in = 0;
volatile size_t got_arg = 0;

void hello(void* arg)
{
  ran_in = pthread_self();
  got_arg = (size_t) arg;
  calls++;
}

void* worker(void* arg)
{
  size_t local = (size_t) arg;              // must survive the interruption
  while (!stop)
    sched_yield();                          // a syscall in every round: the call is delivered here
  return (void*) (local + 1);
}

int waitForCalls(int n)
{
  for (int i = 0; i < 100000 && calls < n; ++i)
    sched_yield();
  return calls >= n;
}

int main()
{
  pthread_t thread;
  pthread_create(&thread, 0, worker, (void*) 41);

  printf(pthread_invoke_function(thread, hello, (void*) 7) == 0 ? "[PASS] invoke accepted\n" : "[FAIL] invoke refused\n");
  int ran = waitForCalls(1);
  printf(ran ? "[PASS] the function ran\n" : "[FAIL] the function never ran\n");
  printf(ran && ran_in == thread ? "[PASS] it ran in the other thread\n" : "[FAIL] it ran in the wrong thread\n");
  printf(got_arg == 7 ? "[PASS] the argument arrived\n" : "[FAIL] wrong argument\n");

  int accepted = 0;
  for (int i = 0; i < 1000 && !accepted; ++i)  // refused while the first call is still pending
    accepted = pthread_invoke_function(thread, hello, (void*) 8) == 0;
  printf(accepted && waitForCalls(2) && got_arg == 8 ? "[PASS] a second invoke works too\n"
                                                     : "[FAIL] second invoke\n");

  printf(pthread_invoke_function(pthread_self(), hello, 0) == -1 ? "[PASS] invoking the caller fails\n"
                                                                 : "[FAIL] invoked the caller\n");
  printf(pthread_invoke_function(thread, 0, 0) == -1 ? "[PASS] NULL function rejected\n" : "[FAIL] NULL accepted\n");
  printf(pthread_invoke_function(9999, hello, 0) == -1 ? "[PASS] unknown thread rejected\n"
                                                       : "[FAIL] unknown thread accepted\n");

  stop = 1;
  void* value = 0;
  pthread_join(thread, &value);
  printf((size_t) value == 42 ? "[PASS] the thread continued where it was, its local intact\n"
                              : "[FAIL] the thread did not continue correctly\n");
  return 0;
}
