/*
 * uthread.c - Module 10: reference solution (parts A, B and the bonus C).
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

/* ============================== Part A ============================== */

/* An exited detached thread whose stack was still in use when it exited. */
static struct uthread *reap_me;

static void after_switch(void)
{
  if (reap_me) {
    release(reap_me);
    reap_me = NULL;
  }
}

static void schedule(void)
{
  if (ready.len > 0) {
    switch_to(find(q_pop(&ready)));
    return;
  }
  for (int i = 0; i < UTHREAD_MAX; i++)
    if (threads[i].state == BLOCKED) {
      fprintf(stderr, "uthread: deadlock - every thread is blocked\n");
      exit(UTHREAD_DEADLOCK_EXIT);
    }
  exit(0);                                   /* the last thread has ended */
}

static void trampoline(void)
{
  after_switch();
  /* Created with "interrupts" off (the context was captured inside
   * uthread_create): switch them on for the thread's own code. */
  sigset_t none;
  sigemptyset(&none);
  irq_restore(none);
  uthread_exit(current->start(current->arg));
}

int uthread_create(uthread_t *thread, void *(*start)(void *), void *arg)
{
  init();
  if (!thread || !start)
    return EINVAL;
  sigset_t irq = irq_off();
  struct uthread *t = free_slot();
  void *stack = t ? stack_alloc() : NULL;
  if (!stack) {
    irq_restore(irq);
    return EAGAIN;
  }
  memset(t, 0, sizeof *t);
  getcontext(&t->ctx);
  t->ctx.uc_stack.ss_sp = stack_base(stack);
  t->ctx.uc_stack.ss_size = UTHREAD_STACK_SIZE;
  t->ctx.uc_link = NULL;
  makecontext(&t->ctx, trampoline, 0);
  t->id = next_id++;
  t->stack = stack;
  t->start = start;
  t->arg = arg;
  t->state = READY;
  q_push(&ready, t->id);
  *thread = t->id;
  irq_restore(irq);
  return 0;
}

uthread_t uthread_self(void)
{
  init();
  return current->id;
}

void uthread_yield(void)
{
  init();
  sigset_t irq = irq_off();
  if (ready.len > 0) {
    current->state = READY;
    q_push(&ready, current->id);
    schedule();
  }
  irq_restore(irq);
}

_Noreturn void uthread_exit(void *retval)
{
  init();
  irq_off();                                 /* for good: we never come back */
  struct uthread *self = current;
  self->retval = retval;
  self->state = ZOMBIE;
  if (self->joiner) {
    self->joiner->joining = NULL;
    self->joiner->state = READY;
    q_push(&ready, self->joiner->id);
  }
  if (self->detached)
    reap_me = self;                          /* the next thread frees us */
  schedule();
  abort();                                   /* a ZOMBIE is never resumed */
}

int uthread_join(uthread_t thread, void **retval)
{
  init();
  sigset_t irq = irq_off();
  int err = 0;
  struct uthread *t = find(thread);
  if (!t)
    err = ESRCH;
  else if (t == current)
    err = EDEADLK;
  else if (t->detached || t->joiner)
    err = EINVAL;
  else
    for (struct uthread *w = t; w; w = w->joining)
      if (w == current)
        err = EDEADLK;                       /* t waits (via others) for us */
  if (err) {
    irq_restore(irq);
    return err;
  }
  if (t->state != ZOMBIE) {
    t->joiner = current;
    current->joining = t;
    current->state = BLOCKED;
    schedule();                              /* t's exit wakes us */
  }
  if (retval)
    *retval = t->retval;
  release(t);                                /* t has switched away for good */
  irq_restore(irq);
  return 0;
}

int uthread_detach(uthread_t thread)
{
  init();
  sigset_t irq = irq_off();
  int err = 0;
  struct uthread *t = find(thread);
  if (!t)
    err = ESRCH;
  else if (t->detached || t->joiner)
    err = EINVAL;
  else if (t->state == ZOMBIE)
    release(t);                              /* nobody will ever join it */
  else
    t->detached = 1;
  irq_restore(irq);
  return err;
}

/* ============================== Part B ============================== */

static void wake(uthread_t id)
{
  struct uthread *t = find(id);
  t->state = READY;
  q_push(&ready, id);
}

/* Caller holds "interrupts" off. */
static int lock_locked(uthread_mutex_t *m)
{
  if (m->owner == current->id)
    return EDEADLK;
  if (m->owner == -1) {
    m->owner = current->id;
    return 0;
  }
  q_push(&m->waiters, current->id);
  current->state = BLOCKED;
  schedule();                                /* unlock made us the owner */
  return 0;
}

static int unlock_locked(uthread_mutex_t *m)
{
  if (m->owner != current->id)
    return EPERM;
  if (m->waiters.len > 0) {
    m->owner = q_pop(&m->waiters);           /* hand over: no barging */
    wake(m->owner);
  } else {
    m->owner = -1;
  }
  return 0;
}

int uthread_mutex_lock(uthread_mutex_t *m)
{
  init();
  sigset_t irq = irq_off();
  int err = lock_locked(m);
  irq_restore(irq);
  return err;
}

int uthread_mutex_trylock(uthread_mutex_t *m)
{
  init();
  sigset_t irq = irq_off();
  int err = 0;
  if (m->owner != -1)
    err = EBUSY;
  else
    m->owner = current->id;
  irq_restore(irq);
  return err;
}

int uthread_mutex_unlock(uthread_mutex_t *m)
{
  init();
  sigset_t irq = irq_off();
  int err = unlock_locked(m);
  irq_restore(irq);
  return err;
}

int uthread_cond_wait(uthread_cond_t *c, uthread_mutex_t *m)
{
  init();
  sigset_t irq = irq_off();
  if (m->owner != current->id) {
    irq_restore(irq);
    return EPERM;
  }
  /* Queue up, unlock and block without a switch in between: nobody can
   * signal between our unlock and our sleep (no lost wake-up). Without
   * preemption that is automatic; with it, irq_off() guarantees it. */
  q_push(&c->waiters, current->id);
  unlock_locked(m);
  current->state = BLOCKED;
  schedule();
  lock_locked(m);
  irq_restore(irq);
  return 0;
}

int uthread_cond_signal(uthread_cond_t *c)
{
  init();
  sigset_t irq = irq_off();
  if (c->waiters.len > 0)
    wake(q_pop(&c->waiters));
  irq_restore(irq);
  return 0;
}

int uthread_cond_broadcast(uthread_cond_t *c)
{
  init();
  sigset_t irq = irq_off();
  while (c->waiters.len > 0)
    wake(q_pop(&c->waiters));
  irq_restore(irq);
  return 0;
}

/* ========================= Part C: preemption ========================= */

/* The "timer interrupt". SIGVTALRM is blocked while it runs, so the
 * library's data is safe; the thread we switch to restores its own mask
 * (swapcontext saves and restores the signal mask with the registers). */
static void on_tick(int sig)
{
  (void)sig;
  if (ready.len > 0) {
    current->state = READY;
    q_push(&ready, current->id);
    schedule();
  }
}

int uthread_preempt(long usec)
{
  init();
  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = on_tick;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGVTALRM, &sa, NULL);
  struct itimerval it = {{usec / 1000000, usec % 1000000}, {usec / 1000000, usec % 1000000}};
  setitimer(ITIMER_VIRTUAL, &it, NULL);      /* usec == 0 stops the timer */
  return 0;
}
