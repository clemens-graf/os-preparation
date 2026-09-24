/*
 * generator.c - a coroutine: a function that can stop in the middle and
 * be resumed later, with all its local variables intact.
 *
 * next_fib() does not recompute anything: it switches back into fib(),
 * which continues its loop right after its last "yield". Cooperative
 * threads (and your uthread library) are exactly this, plus a scheduler
 * that decides whom to resume.
 */
#include <stdio.h>
#include <ucontext.h>

static ucontext_t caller, gen;
static long value;
static char stack[64 * 1024];

static void yield_value(long v)
{
  value = v;
  swapcontext(&gen, &caller);          /* back to whoever called next_fib */
}

static void fib(void)
{
  long a = 0, b = 1;                   /* survive across yields: they are on gen's stack */
  for (;;) {
    yield_value(a);
    long next = a + b;
    a = b;
    b = next;
  }
}

static long next_fib(void)
{
  swapcontext(&caller, &gen);          /* resume fib where it stopped */
  return value;
}

int main(void)
{
  getcontext(&gen);
  gen.uc_stack.ss_sp = stack;
  gen.uc_stack.ss_size = sizeof stack;
  gen.uc_link = NULL;
  makecontext(&gen, fib, 0);

  printf("first 15 Fibonacci numbers from a generator:\n ");
  for (int i = 0; i < 15; i++)
    printf(" %ld", next_fib());
  printf("\n");
  return 0;
}
