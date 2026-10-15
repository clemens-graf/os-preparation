#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define ITEMS 2000

sem_t items;          // counts produced, not yet consumed items
sem_t guard;          // used as a lock (starts at 1)
volatile size_t produced = 0;
volatile size_t counter = 0;

void* producer(void* arg)
{
  (void) arg;
  for (int i = 0; i < ITEMS; ++i)
  {
    produced++;
    sem_post(&items);
  }
  return 0;
}

void* increment(void* arg)
{
  (void) arg;
  for (int i = 0; i < 10000; ++i)
  {
    sem_wait(&guard);
    size_t value = counter;
    for (volatile int j = 0; j < 10; ++j) {}
    counter = value + 1;
    sem_post(&guard);
  }
  return 0;
}

int main()
{
  sem_t s;
  printf(sem_init(&s, 0, 2) == 0 ? "[PASS] init with 2\n" : "[FAIL] sem_init failed\n");
  printf(sem_trywait(&s) == 0 && sem_trywait(&s) == 0 ? "[PASS] two trywaits succeed\n"
                                                      : "[FAIL] trywait failed on a positive value\n");
  printf(sem_trywait(&s) != 0 ? "[PASS] third trywait fails\n" : "[FAIL] trywait went below 0\n");
  sem_post(&s);
  printf(sem_trywait(&s) == 0 ? "[PASS] post makes one available\n" : "[FAIL] post did not count\n");

  sem_init(&items, 0, 0);
  pthread_t p;
  pthread_create(&p, 0, producer, 0);
  for (int i = 0; i < ITEMS; ++i)
    sem_wait(&items);                         // blocks until the producer posted
  pthread_join(p, 0);
  printf(produced == ITEMS ? "[PASS] waited for every item\n" : "[FAIL] consumed before produced\n");

  sem_init(&guard, 0, 1);
  pthread_t t[4];
  for (int i = 0; i < 4; ++i)
    pthread_create(&t[i], 0, increment, 0);
  for (int i = 0; i < 4; ++i)
    pthread_join(t[i], 0);
  printf(counter == 40000 ? "[PASS] semaphore as a lock: no increment lost\n"
                          : "[FAIL] %zu of 40000 increments\n", counter);

  printf(sem_wait(0) != 0 && sem_post(0) != 0 ? "[PASS] NULL rejected\n" : "[FAIL] NULL accepted\n");
  return 0;
}
