/*
 * shared_vs_private.c - what do threads share?
 *
 *   shared by all threads of a process      private to each thread
 *   ----------------------------------      ----------------------
 *   code, global & static variables          registers (incl. PC, SP)
 *   heap (malloc)                            stack (local variables)
 *   open file descriptors, cwd, PID          thread ID, errno, signal mask
 *
 * "Private stack" does not mean "protected stack": all stacks live in the
 * same address space, so a thread CAN read and write another thread's
 * locals if it has a pointer to them. Private only means "each thread has
 * its own region to push its frames onto".
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static int global_counter = 0;     /* one copy for the whole process */
static int *heap_value;            /* the pointer AND the int are shared */
static int *main_local_ptr;        /* points into main's stack */
static pthread_barrier_t all_alive;

static void *show(void *arg)
{
  int local = 0;                   /* one copy per thread, on its own stack */
  const char *name = arg;
  /* Wait until all three threads exist at the same time. Otherwise the
   * library could reuse a finished thread's stack for the next one and
   * the addresses would look identical. */
  pthread_barrier_wait(&all_alive);
  printf("%-8s &local=%p  &global=%p  heap=%p\n",
         name, (void *)&local, (void *)&global_counter, (void *)heap_value);
  return NULL;
}

static void *poke_main(void *arg)
{
  (void)arg;
  *main_local_ptr = 99;            /* writing into ANOTHER thread's stack frame */
  return NULL;
}

int main(void)
{
  heap_value = malloc(sizeof *heap_value);
  int main_local = 1;
  main_local_ptr = &main_local;

  pthread_barrier_init(&all_alive, NULL, 3);
  pthread_t a, b;
  pthread_create(&a, NULL, show, "thread A");
  pthread_create(&b, NULL, show, "thread B");
  show("main");
  pthread_join(a, NULL);
  pthread_join(b, NULL);
  pthread_barrier_destroy(&all_alive);

  /* The &local values differ (the two thread stacks are megabytes apart
   * and main's stack is somewhere else entirely). &global and heap are
   * identical for everyone. */

  pthread_t c;
  pthread_create(&c, NULL, poke_main, NULL);
  pthread_join(c, NULL);
  printf("main_local was changed by another thread: %d\n", main_local);

  free(heap_value);
  return 0;
}
