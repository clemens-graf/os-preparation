/*
 * semaphores.c - POSIX semaphores in their three classic roles.
 *
 * A semaphore is a counter that never goes below zero:
 *   sem_wait (P, "down"): if the value is > 0, decrement it; otherwise
 *                         sleep until it is > 0, then decrement.
 *   sem_post (V, "up"):   increment; wake one sleeper if any.
 * Unlike a mutex, it has no owner: any thread may post.
 *
 * Role 1 - mutual exclusion:     initial value 1  ("binary semaphore")
 * Role 2 - signalling / ordering: initial value 0 ("wait until X happened")
 * Role 3 - counting resources:   initial value N  (N identical slots)
 *
 * The bounded buffer below uses all three.
 */
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#define CAP 4
#define ITEMS 1000
#define PRODUCERS 3
#define CONSUMERS 3

static int buf[CAP];
static int head, tail;
static sem_t mutex;      /* role 1: protects head/tail/buf, initial 1       */
static sem_t empty;      /* role 3: free slots,                initial CAP  */
static sem_t full;       /* role 3: filled slots,              initial 0    */
static sem_t started;    /* role 2: "a consumer is up",        initial 0    */

static long consumed_sum;

static void *producer(void *arg)
{
  (void)arg;
  for (int i = 1; i <= ITEMS; i++) {
    sem_wait(&empty);          /* reserve a free slot (may sleep)    */
    sem_wait(&mutex);
    buf[tail] = i;
    tail = (tail + 1) % CAP;
    sem_post(&mutex);
    sem_post(&full);           /* announce a filled slot             */
  }
  return NULL;
}

static void *consumer(void *arg)
{
  (void)arg;
  sem_post(&started);
  long sum = 0;
  for (int i = 0; i < ITEMS * PRODUCERS / CONSUMERS; i++) {
    sem_wait(&full);           /* wait for a filled slot             */
    sem_wait(&mutex);
    int x = buf[head];
    head = (head + 1) % CAP;
    sem_post(&mutex);
    sem_post(&empty);          /* the slot is free again             */
    sum += x;
  }
  sem_wait(&mutex);            /* reuse the binary semaphore as a lock */
  consumed_sum += sum;
  sem_post(&mutex);
  return NULL;
}

int main(void)
{
  sem_init(&mutex, 0, 1);
  sem_init(&empty, 0, CAP);
  sem_init(&full, 0, 0);
  sem_init(&started, 0, 0);

  pthread_t p[PRODUCERS], c[CONSUMERS];
  for (int i = 0; i < CONSUMERS; i++)
    pthread_create(&c[i], NULL, consumer, NULL);
  for (int i = 0; i < CONSUMERS; i++)
    sem_wait(&started);        /* role 2: wait until all consumers run */
  printf("all %d consumers are running\n", CONSUMERS);

  for (int i = 0; i < PRODUCERS; i++)
    pthread_create(&p[i], NULL, producer, NULL);
  for (int i = 0; i < PRODUCERS; i++)
    pthread_join(p[i], NULL);
  for (int i = 0; i < CONSUMERS; i++)
    pthread_join(c[i], NULL);

  printf("consumed sum = %ld (expected %ld)\n", consumed_sum,
         (long)PRODUCERS * ITEMS * (ITEMS + 1) / 2);
  return 0;
}
