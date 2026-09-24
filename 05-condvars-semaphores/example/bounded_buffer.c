/*
 * bounded_buffer.c - producer/consumer with a mutex and two condition
 * variables. THE canonical condition-variable program; learn its shape.
 *
 * A condition variable lets a thread sleep until "something it cares
 * about" may have changed. The rules, all visible below:
 *
 *  1. The condition (count == CAP, count == 0) is ordinary shared state,
 *     protected by the mutex. The condition variable itself stores nothing.
 *  2. Always wait in a WHILE loop that re-checks the condition:
 *        while (!condition) pthread_cond_wait(&cv, &m);
 *     Wake-ups can be spurious, and another thread may have "stolen" the
 *     slot between the signal and our wake-up (Mesa semantics).
 *  3. pthread_cond_wait atomically releases the mutex AND goes to sleep,
 *     and re-acquires the mutex before returning. That atomicity is what
 *     makes lost wake-ups impossible (see lost_wakeup.c).
 *  4. Change the state, then signal, while holding the mutex.
 */
#include <pthread.h>
#include <stdio.h>

#define CAP 4
#define ITEMS 20

static int buf[CAP];
static int head, tail, count;                /* ring buffer state */
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

static void put(int x)
{
  pthread_mutex_lock(&m);
  while (count == CAP)                       /* rule 2: while, not if */
    pthread_cond_wait(&not_full, &m);        /* rule 3: sleeps with m released */
  buf[tail] = x;
  tail = (tail + 1) % CAP;
  count++;
  pthread_cond_signal(&not_empty);           /* rule 4: a consumer may proceed */
  pthread_mutex_unlock(&m);
}

static int get(void)
{
  pthread_mutex_lock(&m);
  while (count == 0)
    pthread_cond_wait(&not_empty, &m);
  int x = buf[head];
  head = (head + 1) % CAP;
  count--;
  pthread_cond_signal(&not_full);            /* a producer may proceed */
  pthread_mutex_unlock(&m);
  return x;
}

static void *producer(void *arg)
{
  (void)arg;
  for (int i = 1; i <= ITEMS; i++) {
    put(i);
    printf("produced %2d\n", i);
  }
  put(-1);                                   /* sentinel: "no more items" */
  return NULL;
}

static void *consumer(void *arg)
{
  (void)arg;
  long sum = 0;
  for (;;) {
    int x = get();
    if (x < 0)
      break;
    printf("                consumed %2d\n", x);
    sum += x;
  }
  printf("consumer: sum = %ld (expected %d)\n", sum, ITEMS * (ITEMS + 1) / 2);
  return NULL;
}

int main(void)
{
  pthread_t p, c;
  pthread_create(&p, NULL, producer, NULL);
  pthread_create(&c, NULL, consumer, NULL);
  pthread_join(p, NULL);
  pthread_join(c, NULL);
  return 0;
}
