/*
 * uthread.c - Module 10 assignment. The plumbing (thread table, queues,
 * stacks, the context switch) is given; replace the TODOs.
 * Test:  make test-threads (part A)   make test-sync (part B)   make test-preempt (bonus)
 *
 * The life of a thread:
 *
 *    create             schedule()            exit / return
 *   -------> READY ---------------> RUNNING ----------------> ZOMBIE --+
 *              ^                     |  |                               | join or
 *              |      yield          |  |  join / lock / cond_wait      | detach
 *              +---------------------+  v                               v
 *              +-------- wake ------ BLOCKED                           FREE
 *
 * SWEB has the same states (Thread::state_: Running, Sleeping,
 * ToBeDestroyed) and the same problem of a thread that cannot free its own
 * stack - that is what SWEB's cleanup thread is for.
 */
#include "uthread.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <ucontext.h>
#include <unistd.h>

enum state { FREE, READY, RUNNING, BLOCKED, ZOMBIE };

struct uthread {
  uthread_t id;
  enum state state;
  ucontext_t ctx;              /* saved registers while not running */
  void *stack;                 /* from stack_alloc(); NULL for main */
  void *(*start)(void *);
  void *arg;
  void *retval;                /* set by exit, collected by join */
  int detached;
  struct uthread *joiner;      /* the thread blocked in join() on this one */
  struct uthread *joining;     /* the thread this one is blocked on in join() */
};

static struct uthread threads[UTHREAD_MAX];
static struct uthread *current;          /* the RUNNING thread */
static uthread_t next_id = 1;
static struct uthread_queue ready;       /* ids of READY threads, FIFO */

/* =============================== given =============================== */

static void q_push(struct uthread_queue *q, uthread_t id)
{
  q->item[(q->head + q->len) % UTHREAD_MAX] = id;
  q->len++;
}

static uthread_t q_pop(struct uthread_queue *q)
{
  uthread_t id = q->item[q->head];
  q->head = (q->head + 1) % UTHREAD_MAX;
  q->len--;
  return id;
}

/* The thread with this id, or NULL (never existed / already freed). */
static struct uthread *find(uthread_t id)
{
  for (int i = 0; i < UTHREAD_MAX; i++)
    if (threads[i].state != FREE && threads[i].id == id)
      return &threads[i];
  return NULL;
}

static struct uthread *free_slot(void)
{
  for (int i = 0; i < UTHREAD_MAX; i++)
    if (threads[i].state == FREE)
      return &threads[i];
  return NULL;
}

/* The first call into the library turns the caller into thread 0. */
static void init(void)
{
  if (current)
    return;
  current = &threads[0];
  current->id = 0;
  current->state = RUNNING;
}

/* Stacks: UTHREAD_STACK_SIZE usable bytes above one PROT_NONE guard page.
 * Stacks grow downwards, so an overflow hits the guard page and crashes
 * cleanly instead of overwriting whatever lies below. */
static int live_stacks;

static size_t page_size(void)
{
  return (size_t)sysconf(_SC_PAGESIZE);
}

static void *stack_alloc(void)
{
  void *p = mmap(NULL, UTHREAD_STACK_SIZE + page_size(), PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED)
    return NULL;
  mprotect(p, page_size(), PROT_NONE);
  live_stacks++;
  return p;
}

/* The usable part of a stack from stack_alloc(), for uc_stack. */
static void *stack_base(void *stack)
{
  return (char *)stack + page_size();
}

static void stack_free(void *stack)
{
  munmap(stack, UTHREAD_STACK_SIZE + page_size());
  live_stacks--;
}

int uthread_live_stacks(void)
{
  return live_stacks;
}

/* Free a finished thread: its stack and its slot. Never call this for the
 * RUNNING thread - it would pull the stack out from under its own feet. */
static void release(struct uthread *t)
{
  if (t->stack)
    stack_free(t->stack);
  memset(t, 0, sizeof *t);       /* state = FREE */
}

/* "Interrupts" for part C: while SIGVTALRM is blocked, no preemption. */
static sigset_t irq_off(void)
{
  sigset_t alrm, old;
  sigemptyset(&alrm);
  sigaddset(&alrm, SIGVTALRM);
  sigprocmask(SIG_BLOCK, &alrm, &old);
  return old;
}

static void irq_restore(sigset_t old)
{
  sigprocmask(SIG_SETMASK, &old, NULL);
}

static void after_switch(void);

/*
 * Give the CPU to `to`. The caller must already have set its own state
 * (READY and queued, BLOCKED, or ZOMBIE). Returns when somebody switches
 * back to the caller - for a ZOMBIE, never.
 */
static void switch_to(struct uthread *to)
{
  struct uthread *from = current;
  current = to;
  to->state = RUNNING;
  if (from != to)
    swapcontext(&from->ctx, &to->ctx);
  after_switch();                /* runs in the thread that now has the CPU */
}

/* ============================ Part A: TODO ============================ */

/* Runs right after every switch, in the thread that just got the CPU. */
static void after_switch(void)
{
  /* TODO: free a detached thread that has just exited. It could not free
   * its own stack (it was running on it); now it is safe. */
}

/*
 * Pick the next thread and switch to it. The caller has already set its
 * own state (and queued itself if it is READY).
 */
static void schedule(void)
{
  /* TODO:
   * - ready queue not empty: pop the front, switch_to() it;
   * - empty, but some thread is BLOCKED: deadlock - print "uthread:
   *   deadlock" to stderr and exit(UTHREAD_DEADLOCK_EXIT);
   * - empty and nobody blocked: the last thread has ended - exit(0). */
  (void)q_pop;
  (void)switch_to;
  exit(UTHREAD_DEADLOCK_EXIT);
}

/* Every new thread starts here (makecontext). */
static void trampoline(void)
{
  /* TODO: after_switch() first - we got here by a switch, too. Then run
   * the start routine; returning from it must behave like uthread_exit. */
}

int uthread_create(uthread_t *thread, void *(*start)(void *), void *arg)
{
  init();
  /* TODO: slot + stack (EAGAIN if either is missing), then
   *   getcontext(&t->ctx);
   *   t->ctx.uc_stack.ss_sp = stack_base(stack);
   *   t->ctx.uc_stack.ss_size = UTHREAD_STACK_SIZE;
   *   t->ctx.uc_link = NULL;          (trampoline never returns)
   *   makecontext(&t->ctx, trampoline, 0);
   * fill in the rest, append to the ready queue. */
  (void)free_slot;
  (void)stack_alloc;
  (void)stack_base;
  (void)trampoline;
  (void)next_id;
  return ENOSYS;
}

uthread_t uthread_self(void)
{
  init();
  /* TODO */
  return -1;
}

void uthread_yield(void)
{
  init();
  /* TODO */
  (void)ready;
  (void)q_push;
  (void)schedule;
}

_Noreturn void uthread_exit(void *retval)
{
  init();
  /* TODO: become a ZOMBIE with retval; wake the joiner, if any; a
   * detached thread must be freed - but not by itself. Then schedule(). */
  (void)release;
  fprintf(stderr, "uthread_exit: not implemented\n");
  abort();
}

int uthread_join(uthread_t thread, void **retval)
{
  init();
  /* TODO: the error checks from uthread.h (for the cycle, follow the
   * `joining` pointers starting at the target); if the target is not a
   * ZOMBIE yet, block until its exit wakes you; then collect and release. */
  (void)find;
  return ENOSYS;
}

int uthread_detach(uthread_t thread)
{
  init();
  /* TODO */
  return ENOSYS;
}

/* ============================ Part B: TODO ============================ */

int uthread_mutex_lock(uthread_mutex_t *m)
{
  init();
  /* TODO: free -> take it; else queue up in m->waiters and block.
   * When you run again, unlock has already made you the owner. */
  return ENOSYS;
}

int uthread_mutex_trylock(uthread_mutex_t *m)
{
  init();
  /* TODO */
  return ENOSYS;
}

int uthread_mutex_unlock(uthread_mutex_t *m)
{
  init();
  /* TODO: waiters? hand over ownership to the first one and wake it. */
  return ENOSYS;
}

int uthread_cond_wait(uthread_cond_t *c, uthread_mutex_t *m)
{
  init();
  /* TODO: queue up, unlock, block - with no chance for a signal to slip
   * in between (why is that automatic here, and not in part C?) - then
   * lock m again. */
  return ENOSYS;
}

int uthread_cond_signal(uthread_cond_t *c)
{
  init();
  /* TODO */
  return ENOSYS;
}

int uthread_cond_broadcast(uthread_cond_t *c)
{
  init();
  /* TODO */
  return ENOSYS;
}

/* ======================= Part C (bonus): TODO ======================= */

int uthread_preempt(long usec)
{
  /* TODO (bonus): a SIGVTALRM handler that yields, installed with
   * sigaction; setitimer(ITIMER_VIRTUAL, ...) with the interval. Then
   * guard every public function with irq_off()/irq_restore(), and think
   * about the signal mask a brand-new thread starts with. */
  (void)irq_off;
  (void)irq_restore;
  return ENOSYS;
}
