/*
 * return_values.c - three correct ways to get results out of a thread,
 * plus pthread_exit and detached threads.
 *
 * The return value of the start routine is a void *. After the thread has
 * terminated, the pthread library keeps that pointer until someone calls
 * pthread_join (like a zombie process keeps its exit status for wait()).
 */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* (1) Return a heap object. The joiner receives the pointer and owns it. */
static void *make_square(void *arg)
{
  long n = (long)(intptr_t)arg;
  long *res = malloc(sizeof *res);
  *res = n * n;
  return res;
}

/* (2) Small integers can travel inside the pointer value itself. */
static void *add_one(void *arg)
{
  return (void *)((intptr_t)arg + 1);
}

/* (3) Write into a result slot that the caller provided in the argument.
 *     (This is what hello_threads.c does.)
 *
 * pthread_exit(value) ends the calling thread from ANY depth of the call
 * stack - it does not return. Returning from the start routine is
 * equivalent to pthread_exit(return value).
 * Your SWEB pthread_create needs exactly this rule: when start_routine
 * returns, *something* must call pthread_exit with its return value. */
static void level3(void)
{
  pthread_exit((void *)(intptr_t)42);     /* leaves the thread right here */
}

static void level2(void)
{
  level3();
  printf("never printed\n");
}

static void *exits_early(void *arg)
{
  (void)arg;
  level2();
  return NULL;                            /* never reached */
}

/* A detached thread cleans up after itself; nobody may join it. Use it
 * for fire-and-forget work. */
static void *background(void *arg)
{
  (void)arg;
  printf("detached thread running\n");
  return NULL;
}

int main(void)
{
  pthread_t t;
  void *ret;

  pthread_create(&t, NULL, make_square, (void *)(intptr_t)12);
  pthread_join(t, &ret);
  printf("(1) heap result:     %ld\n", *(long *)ret);
  free(ret);

  pthread_create(&t, NULL, add_one, (void *)(intptr_t)41);
  pthread_join(t, &ret);
  printf("(2) integer result:  %ld\n", (long)(intptr_t)ret);

  pthread_create(&t, NULL, exits_early, NULL);
  pthread_join(t, &ret);
  printf("(3) pthread_exit'ed: %ld\n", (long)(intptr_t)ret);

  pthread_create(&t, NULL, background, NULL);
  pthread_detach(t);                      /* joining t now would be an error */
  usleep(100 * 1000);                     /* crude: give it time to print */

  /* Returning from main = exit() = the WHOLE process ends, killing every
   * thread that is still running. Call pthread_exit(NULL) in main instead
   * if the other threads should finish first. */
  return 0;
}
