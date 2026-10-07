#include <stdio.h>
#include <pthread.h>

#define THREADS 4
#define ROUNDS 20

pthread_barrier_t barrier;
volatile int arrived[ROUNDS];
volatile int serial[ROUNDS];
volatile int errors = 0;

void* worker(void* arg)
{
  for (int round = 0; round < ROUNDS; ++round)
  {
    __atomic_add_fetch(&arrived[round], 1, __ATOMIC_SEQ_CST);
    int result = pthread_barrier_wait(&barrier);
    if (result == PTHREAD_BARRIER_SERIAL_THREAD)
      __atomic_add_fetch(&serial[round], 1, __ATOMIC_SEQ_CST);
    else if (result != 0)
      __atomic_add_fetch(&errors, 1, __ATOMIC_SEQ_CST);
    if (arrived[round] != THREADS)       // passed the barrier before everybody arrived?
      __atomic_add_fetch(&errors, 1, __ATOMIC_SEQ_CST);
  }
  return 0;
}

int main()
{
  printf(pthread_barrier_init(&barrier, 0, 0) == -1 ? "[PASS] count 0 rejected\n" : "[FAIL] count 0 accepted\n");
  printf(pthread_barrier_init(&barrier, 0, THREADS) == 0 ? "[PASS] barrier created\n" : "[FAIL] init failed\n");
  pthread_t t[THREADS];
  for (size_t i = 0; i < THREADS; ++i)
    pthread_create(&t[i], 0, worker, 0);
  for (size_t i = 0; i < THREADS; ++i)
    pthread_join(t[i], 0);
  printf(errors == 0 ? "[PASS] nobody passed a barrier early in 20 rounds\n" : "[FAIL] a thread passed too early\n");
  int serial_ok = 1;
  for (int round = 0; round < ROUNDS; ++round)
    serial_ok = serial_ok && serial[round] == 1;
  printf(serial_ok ? "[PASS] exactly one serial thread per round\n" : "[FAIL] serial thread count wrong\n");
  printf(pthread_barrier_destroy(&barrier) == 0 ? "[PASS] destroyed\n" : "[FAIL] destroy failed\n");
  printf(pthread_barrier_wait(&barrier) == -1 ? "[PASS] wait on a destroyed barrier fails\n"
                                              : "[FAIL] destroyed barrier still works\n");
  return 0;
}
