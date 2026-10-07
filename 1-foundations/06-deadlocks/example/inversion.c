/*
 * inversion.c - a deadlock that never happens... in this run.
 *
 * T1 takes A then B, finishes. Only afterwards T2 takes B then A. The two
 * never overlap, so the program always terminates - but with slightly
 * different timing they would deadlock. Tests pass, code ships, and one
 * day it hangs.
 *
 * Lock-order checkers (ThreadSanitizer's deadlock detector, Linux lockdep,
 * the lock checks in SWEB's Lock class, and your ../assignment/lockdep.c)
 * record every "held X while acquiring Y" as an edge X -> Y and report a
 * cycle in that graph - independent of timing.
 *
 *   make tsan
 */
#include <pthread.h>
#include <stdio.h>

static pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;

static void *a_then_b(void *arg)
{
  (void)arg;
  pthread_mutex_lock(&A);
  pthread_mutex_lock(&B);
  pthread_mutex_unlock(&B);
  pthread_mutex_unlock(&A);
  return NULL;
}

static void *b_then_a(void *arg)
{
  (void)arg;
  pthread_mutex_lock(&B);
  pthread_mutex_lock(&A);
  pthread_mutex_unlock(&A);
  pthread_mutex_unlock(&B);
  return NULL;
}

int main(void)
{
  pthread_t t;
  pthread_create(&t, NULL, a_then_b, NULL);
  pthread_join(t, NULL);                 /* T1 is completely done ... */
  pthread_create(&t, NULL, b_then_a, NULL);
  pthread_join(t, NULL);                 /* ... before T2 even starts */
  printf("finished without deadlock (this time)\n");
  return 0;
}
