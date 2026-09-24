/*
 * pingpong.c - the one primitive a thread library needs: switch stacks.
 *
 * A context (ucontext_t) is a saved set of registers - including the stack
 * pointer and the instruction pointer. swapcontext(&a, &b) saves the CPU
 * into a and loads b: execution continues wherever b was saved, on b's
 * stack. That is all a thread switch is; SWEB does the same in assembly
 * (arch_contextSwitch restores a Thread's saved registers).
 *
 * makecontext prepares a context that has never run: a fresh stack and a
 * start function. When that function returns, execution continues at
 * uc_link - or the process exits if uc_link is NULL.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

static ucontext_t main_ctx, ping_ctx, pong_ctx;

static void ping(void)
{
  for (int i = 0; i < 3; i++) {
    int local = 100 + i;               /* lives on ping's own stack */
    printf("ping %d  (local at %p)\n", i, (void *)&local);
    swapcontext(&ping_ctx, &pong_ctx);
  }
  printf("ping done - returning ends this context, uc_link says where to go\n");
}

static void pong(void)
{
  for (int i = 0;; i++) {
    int local = 200 + i;
    printf("  pong %d  (local at %p)\n", i, (void *)&local);
    swapcontext(&pong_ctx, &ping_ctx);
  }
}

static void make(ucontext_t *ctx, void (*fn)(void), ucontext_t *link)
{
  size_t size = 64 * 1024;
  getcontext(ctx);                     /* start from a valid context */
  ctx->uc_stack.ss_sp = malloc(size);  /* a stack of its own */
  ctx->uc_stack.ss_size = size;
  ctx->uc_link = link;
  makecontext(ctx, fn, 0);
}

int main(void)
{
  make(&ping_ctx, ping, &main_ctx);    /* when ping returns: back to main */
  make(&pong_ctx, pong, NULL);         /* pong never returns */
  int local = 0;
  printf("main    (local at %p) - a different stack\n", (void *)&local);
  swapcontext(&main_ctx, &ping_ctx);
  printf("main again. pong is still suspended in the middle of its loop.\n");
  free(ping_ctx.uc_stack.ss_sp);
  free(pong_ctx.uc_stack.ss_sp);       /* safe: nobody runs on it any more */
  return 0;
}
