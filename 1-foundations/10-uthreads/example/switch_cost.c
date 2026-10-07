/*
 * switch_cost.c - what does a thread switch cost?
 *
 *  - a plain function call, for scale;
 *  - swapcontext: a user-level switch (your uthread library). glibc's
 *    swapcontext also saves/restores the signal mask - one system call;
 *  - two kernel threads (pthreads) on ONE CPU handing a token back and
 *    forth through pipes: every hand-over is a trip into the kernel, a
 *    wake-up and a full kernel context switch.
 */
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#define N 200000

static double now(void)
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}

static void __attribute__((noinline)) nothing(volatile int *x)
{
  (*x)++;
}

static ucontext_t a_ctx, b_ctx;
static char b_stack[64 * 1024];

static void b_loop(void)
{
  for (;;)
    swapcontext(&b_ctx, &a_ctx);
}

static int to_b[2], to_a[2];

static void pin(void)
{
  cpu_set_t one;
  CPU_ZERO(&one);
  CPU_SET(0, &one);
  pthread_setaffinity_np(pthread_self(), sizeof one, &one);
}

static void *echo(void *arg)
{
  pin();
  char c;
  for (int i = 0; i < N; i++)
    if (read(to_b[0], &c, 1) != 1 || write(to_a[1], &c, 1) != 1)
      break;
  return NULL;
}

int main(void)
{
  volatile int x = 0;
  double t = now();
  for (int i = 0; i < N; i++)
    nothing(&x);
  double call = (now() - t) / N;

  getcontext(&b_ctx);
  b_ctx.uc_stack.ss_sp = b_stack;
  b_ctx.uc_stack.ss_size = sizeof b_stack;
  makecontext(&b_ctx, b_loop, 0);
  t = now();
  for (int i = 0; i < N; i++)
    swapcontext(&a_ctx, &b_ctx);                 /* two switches per round */
  double user = (now() - t) / N / 2;

  if (pipe(to_b) || pipe(to_a))
    return 1;
  pin();
  pthread_t th;
  pthread_create(&th, NULL, echo, NULL);
  char c = 'x';
  t = now();
  for (int i = 0; i < N; i++)
    if (write(to_b[1], &c, 1) != 1 || read(to_a[0], &c, 1) != 1)
      break;
  double kernel = (now() - t) / N / 2;
  pthread_join(th, NULL);

  printf("function call:                 %7.1f ns\n", call * 1e9);
  printf("swapcontext (user-level):      %7.1f ns per switch\n", user * 1e9);
  printf("kernel threads via pipes:      %7.1f ns per switch (%.0fx)\n", kernel * 1e9,
         kernel / user);
  return 0;
}
