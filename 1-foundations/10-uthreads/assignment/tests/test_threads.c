#include "helpers.h"

/* ------------------------------------------------------------------ */
static uthread_t seen_self;

static void *plus_one(void *arg)
{
  seen_self = uthread_self();
  return (void *)((long)arg + 1);
}

static void test_create_join(void)
{
  CHECK_EQ(uthread_self(), 0);                  /* main is thread 0 */
  uthread_t t;
  REQUIRE(uthread_create(&t, plus_one, (void *)41) == 0, "uthread_create failed");
  CHECK(t > 0, "the new thread got id %d", t);
  CHECK_EQ(seen_self, 0);                       /* it has not run yet */
  void *ret = NULL;
  REQUIRE(uthread_join(t, &ret) == 0, "uthread_join failed");
  CHECK_EQ((long)ret, 42);
  CHECK_EQ(seen_self, t);
  CHECK_EQ(uthread_self(), 0);
  CHECK_EQ(uthread_live_stacks(), 0);           /* joined = freed */

  REQUIRE(uthread_create(&t, plus_one, (void *)1) == 0, "uthread_create failed");
  CHECK_EQ(uthread_join(t, NULL), 0);           /* retval may be NULL */
  CHECK_EQ(uthread_create(NULL, plus_one, NULL), EINVAL);
  CHECK_EQ(uthread_create(&t, NULL, NULL), EINVAL);
}

/* ---- the FIFO ready queue ---- */
static void *three_times(void *arg)
{
  for (int i = 0; i < 3; i++) {
    note((char)(long)arg);
    uthread_yield();
  }
  return NULL;
}

static void test_round_robin_order(void)
{
  trace_reset();
  uthread_t t[3];
  for (int i = 0; i < 3; i++)
    REQUIRE(uthread_create(&t[i], three_times, (void *)(long)('A' + i)) == 0, "create");
  CHECK_TRACE("");                              /* create does not switch */
  note('m');
  uthread_yield();                              /* A, B, C run once each, then main */
  note('m');
  for (int i = 0; i < 3; i++)
    CHECK_EQ(uthread_join(t[i], NULL), 0);
  CHECK_TRACE("mABCmABCABC");
  uthread_yield();                              /* nobody else ready: no-op */
}

/* ---- return == exit ---- */
#pragma GCC diagnostic ignored "-Winfinite-recursion"   /* deep() and depth() below */
static int after_exit;

static void deep(int n)
{
  if (n == 0)
    uthread_exit((void *)7);
  deep(n - 1);
  after_exit = 1;                               /* never reached */
}

static void *exits_deep(void *arg)
{
  deep(20);
  after_exit = 1;
  return (void *)99;
}

static void *returns(void *arg)
{
  return arg;
}

static void test_return_and_exit(void)
{
  uthread_t a, b;
  REQUIRE(uthread_create(&a, exits_deep, NULL) == 0, "create");
  REQUIRE(uthread_create(&b, returns, (void *)123) == 0, "create");
  void *ra = NULL, *rb = NULL;
  CHECK_EQ(uthread_join(a, &ra), 0);
  CHECK_EQ(uthread_join(b, &rb), 0);
  CHECK_EQ((long)ra, 7);
  CHECK_EQ((long)rb, 123);
  CHECK(!after_exit, "code after uthread_exit ran");
}

/* ---- join blocks (does not spin) until the target has finished ---- */
static void *slow(void *arg)
{
  for (int i = 0; i < 5; i++) {
    note('s');
    uthread_yield();
  }
  note('S');
  return NULL;
}

static void *busy(void *arg)
{
  for (int i = 0; i < 5; i++) {
    note('b');
    uthread_yield();
  }
  return NULL;
}

static void test_join_blocks(void)
{
  trace_reset();
  uthread_t s, b;
  REQUIRE(uthread_create(&s, slow, NULL) == 0, "create");
  REQUIRE(uthread_create(&b, busy, NULL) == 0, "create");
  CHECK_EQ(uthread_join(s, NULL), 0);
  note('J');
  CHECK_EQ(uthread_join(b, NULL), 0);
  /* main is BLOCKED in join, not READY: it must not appear in between */
  CHECK_TRACE("sbsbsbsbsbSJ");
}

/* ---- errors ---- */
static uthread_t other;

static void *join_other(void *arg)
{
  return (void *)(long)uthread_join(other, NULL);
}

static void *yield_a_while(void *arg)
{
  for (int i = 0; i < 5; i++)
    uthread_yield();
  return NULL;
}

static void test_join_errors(void)
{
  CHECK_EQ(uthread_join(uthread_self(), NULL), EDEADLK);
  CHECK_EQ(uthread_join(12345, NULL), ESRCH);
  CHECK_EQ(uthread_join(-1, NULL), ESRCH);

  uthread_t t;
  REQUIRE(uthread_create(&t, returns, NULL) == 0, "create");
  CHECK_EQ(uthread_join(t, NULL), 0);
  CHECK_EQ(uthread_join(t, NULL), ESRCH);       /* already joined: gone */

  /* a second joiner */
  uthread_t j;
  REQUIRE(uthread_create(&other, yield_a_while, NULL) == 0, "create");
  REQUIRE(uthread_create(&j, join_other, NULL) == 0, "create");
  uthread_yield();                              /* j is now blocked joining other */
  CHECK_EQ(uthread_join(other, NULL), EINVAL);
  CHECK_EQ(uthread_detach(other), EINVAL);
  void *r;
  CHECK_EQ(uthread_join(j, &r), 0);
  CHECK_EQ((long)r, 0);                         /* j's join succeeded */
  CHECK_EQ(uthread_live_stacks(), 0);
}

/* A joins B, B joins C, C tries to join A: a cycle. */
static uthread_t ta, tb, tc;

static void *join_b(void *arg) { return (void *)(long)uthread_join(tb, NULL); }
static void *join_c(void *arg) { return (void *)(long)uthread_join(tc, NULL); }

static void *join_a_report(void *arg)
{
  long err = uthread_join(ta, NULL);
  note(err == EDEADLK ? 'D' : err == EINVAL ? 'I' : err == 0 ? '0' : '?');
  return NULL;
}

static void test_join_cycle(void)
{
  trace_reset();
  REQUIRE(uthread_create(&ta, join_b, NULL) == 0, "create");
  REQUIRE(uthread_create(&tb, join_c, NULL) == 0, "create");
  REQUIRE(uthread_create(&tc, join_a_report, NULL) == 0, "create");
  uthread_yield();              /* A blocks on B, B on C; C's join of A closes the cycle */
  CHECK(strcmp(trace, "D") == 0, "the third join of the cycle A->B->C->A should fail with "
        "EDEADLK, trace \"%s\" (D = EDEADLK, I = EINVAL, 0 = success)", trace);
  void *r;
  CHECK_EQ(uthread_join(ta, &r), 0);            /* C ended, so B and A could finish */
  CHECK(r == 0, "A's join of B returned %ld", (long)r);
  CHECK_EQ(uthread_join(tb, NULL), ESRCH);      /* joined by A already */
  CHECK_EQ(uthread_join(tc, NULL), ESRCH);      /* joined by B already */
  CHECK_EQ(uthread_live_stacks(), 0);
}

/* ---- detach ---- */
static int finished;

static void *count_and_go(void *arg)
{
  uthread_yield();
  finished++;
  return NULL;
}

static void test_detach(void)
{
  CHECK_EQ(uthread_detach(4242), ESRCH);
  uthread_t t;
  REQUIRE(uthread_create(&t, count_and_go, NULL) == 0, "create");
  CHECK_EQ(uthread_detach(t), 0);
  CHECK_EQ(uthread_detach(t), EINVAL);
  CHECK_EQ(uthread_join(t, NULL), EINVAL);

  /* a thousand detached threads - only possible if they are freed */
  finished = 0;
  int created = 1;
  while (created < 1000) {
    for (int i = 0; i < 20 && created < 1000; i++, created++) {
      if (uthread_create(&t, count_and_go, NULL) != 0) {
        CHECK(0, "uthread_create failed after %d detached threads - are they freed?",
              created);
        return;
      }
      uthread_detach(t);
    }
    while (finished < created)
      uthread_yield();
  }
  CHECK_EQ(finished, 1000);
  CHECK_EQ(uthread_live_stacks(), 0);

  /* detaching a thread that has already ended frees it at once */
  REQUIRE(uthread_create(&t, returns, NULL) == 0, "create");
  uthread_yield();                              /* t runs and becomes a zombie */
  CHECK_EQ(uthread_live_stacks(), 1);
  CHECK_EQ(uthread_detach(t), 0);
  CHECK_EQ(uthread_live_stacks(), 0);
  CHECK_EQ(uthread_join(t, NULL), ESRCH);
}

static void *detach_self(void *arg)
{
  CHECK_EQ(uthread_detach(uthread_self()), 0);
  finished++;
  return NULL;
}

static void test_detach_self(void)
{
  finished = 0;
  uthread_t t;
  for (int round = 0; round < 10; round++) {
    for (int i = 0; i < 30; i++)
      REQUIRE(uthread_create(&t, detach_self, NULL) == 0, "create failed in round %d", round);
    while (finished < 30 * (round + 1))
      uthread_yield();
  }
  CHECK_EQ(uthread_live_stacks(), 0);
}

/* ---- limits and freeing ---- */
static void test_limits(void)
{
  uthread_t t[UTHREAD_MAX];
  int n = 0;
  while (n < UTHREAD_MAX && uthread_create(&t[n], returns, (void *)(long)n) == 0)
    n++;
  CHECK(n == UTHREAD_MAX - 1, "created %d threads next to main, expected %d", n,
        UTHREAD_MAX - 1);
  uthread_t extra;
  CHECK_EQ(uthread_create(&extra, returns, NULL), EAGAIN);
  uthread_yield();                              /* all of them become zombies */
  CHECK_EQ(uthread_create(&extra, returns, NULL), EAGAIN);   /* zombies count */
  for (int i = 0; i < n; i++) {
    void *r;
    CHECK_EQ(uthread_join(t[i], &r), 0);
    CHECK_EQ((long)r, i);
  }
  for (int i = 0; i < 500; i++) {               /* create/join churn */
    REQUIRE(uthread_create(&extra, returns, NULL) == 0, "create failed in round %d", i);
    REQUIRE(uthread_join(extra, NULL) == 0, "join failed in round %d", i);
  }
  CHECK_EQ(uthread_live_stacks(), 0);
  CHECK(extra > 500, "thread ids must not be reused (got %d)", extra);
}

/* ---- every thread has its own stack ---- */
static void *own_stack(void *arg)
{
  char local[4096];
  memset(local, (int)(long)arg, sizeof local);
  for (int i = 0; i < 10; i++)
    uthread_yield();
  for (size_t i = 0; i < sizeof local; i++)
    if (local[i] != (char)(long)arg)
      return (void *)1;
  return NULL;
}

static void test_separate_stacks(void)
{
  uthread_t t[20];
  for (int i = 0; i < 20; i++)
    REQUIRE(uthread_create(&t[i], own_stack, (void *)(long)(i + 1)) == 0, "create");
  for (int i = 0; i < 20; i++) {
    void *r;
    CHECK_EQ(uthread_join(t[i], &r), 0);
    CHECK(r == NULL, "thread %d found its stack overwritten", i);
  }
}

/* ---- in a child process: how the process ends ---- */
static void *print_later(void *arg)
{
  for (int i = 0; i < 3; i++)
    uthread_yield();
  printf("%s", (const char *)arg);
  return NULL;
}

static void main_exits_first(void)
{
  uthread_t t;
  uthread_create(&t, print_later, "one ");
  uthread_create(&t, print_later, "two ");
  uthread_exit(NULL);                           /* main ends, the others go on */
}

static void test_main_exits_first(void)
{
  char out[256];
  int st = in_child(main_exits_first, out, sizeof out);
  CHECK(WIFEXITED(st) && WEXITSTATUS(st) == 0, "expected exit status 0 after the last "
        "thread ended, got %s; output: %s", describe(st), out);
  CHECK(strstr(out, "one ") && strstr(out, "two "), "the other threads did not finish; "
        "output: \"%s\"", out);
}

static int depth(int n)
{
  volatile char pad[256];
  pad[0] = (char)n;
  return depth(n + 1) + pad[0];
}

static void *overflow(void *arg)
{
  return (void *)(long)depth(0);
}

static void stack_overflow(void)
{
  uthread_t t;
  if (uthread_create(&t, overflow, NULL) != 0) {
    printf("create failed\n");
    exit(1);
  }
  uthread_join(t, NULL);
}

static void test_stack_overflow_hits_guard(void)
{
  char out[256];
  int st = in_child(stack_overflow, out, sizeof out);
  CHECK(WIFSIGNALED(st) && WTERMSIG(st) == SIGSEGV, "infinite recursion in a thread should "
        "hit the guard page (SIGSEGV), got %s", describe(st));
}

int main(void)
{
  RUN(test_create_join);
  RUN(test_round_robin_order);
  RUN(test_return_and_exit);
  RUN(test_join_blocks);
  RUN(test_join_errors);
  RUN(test_join_cycle);
  RUN(test_detach);
  RUN(test_detach_self);
  RUN(test_limits);
  RUN(test_separate_stacks);
  RUN(test_main_exits_first);
  RUN(test_stack_overflow_hits_guard);
  return test_summary();
}
