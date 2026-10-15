#include <stdio.h>
#include <pthread.h>
#include <sched.h>

#define ITEMS 500

pthread_mutex_t mutex;
pthread_cond_t not_empty;
volatile int queue = 0;          // items produced, not yet consumed
volatile int consumed = 0;

void* consumer(void* arg)
{
  (void) arg;
  for (int i = 0; i < ITEMS; ++i)
  {
    pthread_mutex_lock(&mutex);
    while (queue == 0)                        // ALWAYS a loop: wake-ups may be spurious
      pthread_cond_wait(&not_empty, &mutex);  // releases the mutex while it waits
    queue--;
    consumed++;
    pthread_mutex_unlock(&mutex);
  }
  return 0;
}

volatile int waiting = 0;
volatile int go = 0;
void* waiter(void* arg)
{
  (void) arg;
  pthread_mutex_lock(&mutex);
  waiting = 1;
  while (!go)
    pthread_cond_wait(&not_empty, &mutex);
  pthread_mutex_unlock(&mutex);               // wait returned WITH the mutex
  return (void*) 1;
}

int main()
{
  printf(pthread_mutex_init(&mutex, 0) == 0 && pthread_cond_init(&not_empty, 0) == 0 ? "[PASS] init\n"
                                                                                      : "[FAIL] init\n");
  pthread_t w;
  pthread_create(&w, 0, waiter, 0);
  while (!waiting)
    sched_yield();
  pthread_mutex_lock(&mutex);                 // hangs here if cond_wait kept the mutex
  printf("[PASS] cond_wait released the mutex\n");
  go = 1;
  pthread_cond_signal(&not_empty);
  pthread_mutex_unlock(&mutex);
  void* value = 0;
  pthread_join(w, &value);
  printf(value == (void*) 1 ? "[PASS] the waiter woke up after the signal\n" : "[FAIL] waiter\n");

  pthread_t c;
  pthread_create(&c, 0, consumer, 0);
  for (int i = 0; i < ITEMS; ++i)
  {
    pthread_mutex_lock(&mutex);
    queue++;
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
    if (i % 50 == 0)
      sched_yield();
  }
  pthread_join(c, 0);
  printf(consumed == ITEMS && queue == 0 ? "[PASS] producer/consumer: every item consumed exactly once\n"
                                         : "[FAIL] %d consumed, %d left\n", consumed, queue);
  printf(pthread_cond_wait(0, &mutex) != 0 && pthread_cond_signal(0) != 0 ? "[PASS] NULL rejected\n"
                                                                          : "[FAIL] NULL accepted\n");
  return 0;
}
