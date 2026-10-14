#include <stdio.h>
#include <pthread.h>

#define THREADS 4
#define ROUNDS 20000

pthread_spinlock_t spin;
pthread_mutex_t mutex;
volatile size_t spin_counter = 0;
volatile size_t mutex_counter = 0;

void* withSpinlock(void* arg)
{
  (void) arg;
  for (int i = 0; i < ROUNDS; ++i)
  {
    pthread_spin_lock(&spin);
    size_t value = spin_counter;              // read - (maybe a thread switch here) - write:
    for (volatile int j = 0; j < 10; ++j) {}  // without a lock, increments get lost
    spin_counter = value + 1;
    pthread_spin_unlock(&spin);
  }
  return 0;
}

void* withMutex(void* arg)
{
  (void) arg;
  for (int i = 0; i < ROUNDS; ++i)
  {
    pthread_mutex_lock(&mutex);
    size_t value = mutex_counter;
    for (volatile int j = 0; j < 10; ++j) {}
    mutex_counter = value + 1;
    pthread_mutex_unlock(&mutex);
  }
  return 0;
}

int main()
{
  printf(pthread_spin_init(&spin, 0) == 0 && pthread_mutex_init(&mutex, 0) == 0 ? "[PASS] init\n"
                                                                                 : "[FAIL] init failed\n");

  printf(pthread_spin_trylock(&spin) == 0 ? "[PASS] trylock on a free lock\n" : "[FAIL] trylock on a free lock\n");
  printf(pthread_spin_trylock(&spin) != 0 ? "[PASS] trylock on a held lock fails\n"
                                          : "[FAIL] trylock took a held lock\n");
  pthread_spin_unlock(&spin);
  printf(pthread_spin_trylock(&spin) == 0 ? "[PASS] free again after unlock\n" : "[FAIL] still locked after unlock\n");
  pthread_spin_unlock(&spin);

  pthread_t threads[THREADS];
  for (int i = 0; i < THREADS; ++i)
    pthread_create(&threads[i], 0, withSpinlock, 0);
  for (int i = 0; i < THREADS; ++i)
    pthread_join(threads[i], 0);
  printf(spin_counter == THREADS * ROUNDS ? "[PASS] spinlock: no increment lost\n"
                                          : "[FAIL] spinlock: %zu of %d increments\n", spin_counter, THREADS * ROUNDS);

  for (int i = 0; i < THREADS; ++i)
    pthread_create(&threads[i], 0, withMutex, 0);
  for (int i = 0; i < THREADS; ++i)
    pthread_join(threads[i], 0);
  printf(mutex_counter == THREADS * ROUNDS ? "[PASS] mutex: no increment lost\n"
                                           : "[FAIL] mutex: %zu of %d increments\n", mutex_counter, THREADS * ROUNDS);

  printf(pthread_spin_lock(0) != 0 && pthread_mutex_lock(0) != 0 ? "[PASS] NULL rejected\n"
                                                                 : "[FAIL] NULL accepted\n");
  return 0;
}
