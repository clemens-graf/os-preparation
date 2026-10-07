#include <stdio.h>
#include <pthread.h>

int global_counter = 0;

void deeper(int depth)
{
  volatile char filler[512];                 // overwrites the stack area of the snapshot's frames
  for (int i = 0; i < 512; ++i)
    filler[i] = (char) depth;
  if (depth > filler[0] - depth)
    deeper(depth - 1);
  else
    pthread_revive();
  printf("[FAIL] pthread_revive returned\n");
}

int main()
{
  printf(pthread_revive() == -1 ? "[PASS] revive without a snapshot fails\n" : "[FAIL] revive without a snapshot\n");

  volatile int local = 1;
  int result = pthread_snapshot();
  global_counter++;
  if (result == 0)
  {
    printf("[PASS] pthread_snapshot returned 0\n");
    local = 2;                               // changed after the snapshot ...
    deeper(5);                               // ... and revived from 6 frames deeper
    printf("[FAIL] after deeper()\n");
    return 1;
  }
  printf(result == 1 ? "[PASS] after revive, pthread_snapshot returns 1\n" : "[FAIL] wrong return value\n");
  printf(local == 1 ? "[PASS] local variable is back to its value at the snapshot\n"
                    : "[FAIL] stack not restored\n");
  printf(global_counter == 2 ? "[PASS] global variable is not restored (incremented twice)\n"
                             : "[FAIL] global counter wrong\n");
  return 0;
}
