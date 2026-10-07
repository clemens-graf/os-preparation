#include <stdio.h>
#include <pthread.h>
#include <sched.h>

// main returns while the thread still runs: the process must stay alive until the thread is done
void* slow(void* arg)
{
  for (int i = 0; i < 20; ++i)
    sched_yield();
  printf("[PASS] thread finished after main returned\n");
  return 0;
}

int main()
{
  pthread_t id;
  pthread_create(&id, 0, slow, 0);
  printf("[INFO] main returns now\n");
  return 0;
}
