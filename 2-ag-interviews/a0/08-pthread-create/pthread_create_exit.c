#include <stdio.h>
#include <pthread.h>
#include <sched.h>

// main ends with pthread_exit while the thread still runs: the process must stay alive until
// the thread is done. (Returning from main would call exit() and end every thread - POSIX.)
void* slow(void* arg)
{
  for (int i = 0; i < 20; ++i)
    sched_yield();
  printf("[PASS] thread finished after main called pthread_exit\n");
  return 0;
}

int main()
{
  pthread_t id;
  pthread_create(&id, 0, slow, 0);
  printf("[INFO] main calls pthread_exit now\n");
  pthread_exit(0);
}
