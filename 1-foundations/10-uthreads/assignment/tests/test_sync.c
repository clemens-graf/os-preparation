#include "helpers.h"

/* ------------------------------ mutex ------------------------------ */
static uthread_mutex_t shared = UTHREAD_MUTEX_INITIALIZER;

static void *unlock_foreign(void *arg)
{
  return (void *)(long)uthread_mutex_unlock(&shared);
}

static void *trylock_foreign(void *arg)
{
  return (void *)(long)uthread_mutex_trylock(&shared);
}

static void test_mutex_errors(void)
{
  uthread_mutex_t m = UTHREAD_MUTEX_INITIALIZER;
  CHECK_EQ(uthread_mutex_lock(&m), 0);
  CHECK_EQ(uthread_mutex_lock(&m), EDEADLK);
  CHECK_EQ(uthread_mutex_trylock(&m), EBUSY);
  CHECK_EQ(uthread_mutex_unlock(&m), 0);
  CHECK_EQ(uthread_mutex_unlock(&m), EPERM);
  CHECK_EQ(uthread_mutex_trylock(&m), 0);
  CHECK_EQ(uthread_mutex_unlock(&m), 0);

  CHECK_EQ(uthread_mutex_lock(&shared), 0);
  uthread_t t;
  void *r;
  REQUIRE(uthread_create(&t, unlock_foreign, NULL) == 0, "create");
  CHECK_EQ(uthread_join(t, &r), 0);
  CHECK_EQ((long)r, EPERM);                   /* not the owner */
  REQUIRE(uthread_create(&t, trylock_foreign, NULL) == 0, "create");
  CHECK_EQ(uthread_join(t, &r), 0);
  CHECK_EQ((long)r, EBUSY);
  CHECK_EQ(uthread_mutex_unlock(&shared), 0);
}

/* A yield inside the critical section: without a working mutex, the
 * read-yield-write sequence loses updates. */
static long counter;
static uthread_mutex_t counter_lock = UTHREAD_MUTEX_INITIALIZER;

static void *increment(void *arg)
{
  for (int i = 0; i < 50; i++) {
    uthread_mutex_lock(&counter_lock);
    long v = counter;
    uthread_yield();
    counter = v + 1;
    uthread_mutex_unlock(&counter_lock);
    uthread_yield();
  }
  return NULL;
}

static void test_mutual_exclusion(void)
{
  counter = 0;
  uthread_t t[8];
  for (int i = 0; i < 8; i++)
    REQUIRE(uthread_create(&t[i], increment, NULL) == 0, "create");
  for (int i = 0; i < 8; i++)
    CHECK_EQ(uthread_join(t[i], NULL), 0);
  CHECK_EQ(counter, 400);
  CHECK_EQ(counter_lock.owner, -1);
}

/* Unlock hands the mutex to the first waiter; the unlocker queues up. */
static uthread_mutex_t fifo = UTHREAD_MUTEX_INITIALIZER;

static void *take_turn(void *arg)
{
  char me = (char)(long)arg;
  note(me);                                   /* arriving */
  uthread_mutex_lock(&fifo);
  note((char)(me + 'a' - 'A'));               /* got it */
  uthread_mutex_unlock(&fifo);
  return NULL;
}

static void test_fifo_handoff(void)
{
  trace_reset();
  REQUIRE(uthread_mutex_lock(&fifo) == 0, "lock");
  uthread_t t[3];
  for (int i = 0; i < 3; i++)
    REQUIRE(uthread_create(&t[i], take_turn, (void *)(long)('B' + i)) == 0, "create");
  uthread_yield();                            /* B, C, D queue up on the mutex */
  uthread_mutex_unlock(&fifo);                /* -> owned by B */
  uthread_mutex_lock(&fifo);                  /* main must wait behind C and D */
  note('m');
  uthread_mutex_unlock(&fifo);
  for (int i = 0; i < 3; i++)
    CHECK_EQ(uthread_join(t[i], NULL), 0);
  CHECK_TRACE("BCDbcdm");
}

/* -------------------------- condition variables -------------------------- */
static uthread_mutex_t cm = UTHREAD_MUTEX_INITIALIZER;
static uthread_cond_t cv = UTHREAD_COND_INITIALIZER;
static int go;

static void *wait_for_go(void *arg)
{
  uthread_mutex_lock(&cm);
  while (!go)
    uthread_cond_wait(&cv, &cm);
  note((char)(long)arg);
  int owner_ok = cm.owner == uthread_self();  /* wait re-locks before returning */
  uthread_mutex_unlock(&cm);
  return (void *)(long)owner_ok;
}

static void test_cond_signal_order(void)
{
  trace_reset();
  go = 0;
  CHECK_EQ(uthread_cond_signal(&cv), 0);      /* nobody waits: no effect */
  uthread_t t[3];
  for (int i = 0; i < 3; i++)
    REQUIRE(uthread_create(&t[i], wait_for_go, (void *)(long)('1' + i)) == 0, "create");
  uthread_yield();                            /* all three wait */
  CHECK_TRACE("");
  uthread_mutex_lock(&cm);
  go = 1;
  uthread_cond_signal(&cv);                   /* wakes 1 only */
  uthread_mutex_unlock(&cm);
  uthread_yield();
  uthread_yield();
  CHECK_TRACE("1");
  uthread_cond_signal(&cv);                   /* then 2 */
  uthread_yield();
  CHECK_TRACE("12");
  uthread_cond_broadcast(&cv);                /* and the rest */
  for (int i = 0; i < 3; i++) {
    void *ok;
    CHECK_EQ(uthread_join(t[i], &ok), 0);
    CHECK(ok, "thread %d returned from cond_wait without owning the mutex", i + 1);
  }
  CHECK_TRACE("123");
}

static void test_cond_broadcast(void)
{
  trace_reset();
  go = 0;
  uthread_t t[5];
  for (int i = 0; i < 5; i++)
    REQUIRE(uthread_create(&t[i], wait_for_go, (void *)(long)('a' + i)) == 0, "create");
  uthread_yield();
  uthread_mutex_lock(&cm);
  go = 1;
  uthread_cond_broadcast(&cv);
  uthread_mutex_unlock(&cm);
  for (int i = 0; i < 5; i++)
    CHECK_EQ(uthread_join(t[i], NULL), 0);
  CHECK_TRACE("abcde");                       /* in waiting order */
  uthread_mutex_t mine = UTHREAD_MUTEX_INITIALIZER;
  CHECK_EQ(uthread_cond_wait(&cv, &mine), EPERM);
}

/* A bounded buffer: 3 producers, 2 consumers, capacity 2. */
#define ITEMS 30
static int buf[2], buf_len;
static uthread_cond_t not_full = UTHREAD_COND_INITIALIZER, not_empty = UTHREAD_COND_INITIALIZER;
static uthread_mutex_t bm = UTHREAD_MUTEX_INITIALIZER;
static int seen[3 * ITEMS];

static void *producer(void *arg)
{
  for (int i = 0; i < ITEMS; i++) {
    uthread_mutex_lock(&bm);
    while (buf_len == 2)
      uthread_cond_wait(&not_full, &bm);
    buf[buf_len++] = (int)(long)arg * ITEMS + i;
    uthread_cond_signal(&not_empty);
    uthread_mutex_unlock(&bm);
    if (i % 3 == 0)
      uthread_yield();
  }
  return NULL;
}

static void *consumer(void *arg)
{
  for (int i = 0; i < 3 * ITEMS / 2; i++) {
    uthread_mutex_lock(&bm);
    while (buf_len == 0)
      uthread_cond_wait(&not_empty, &bm);
    int item = buf[--buf_len];
    seen[item]++;
    uthread_cond_signal(&not_full);
    uthread_mutex_unlock(&bm);
  }
  return NULL;
}

static void test_bounded_buffer(void)
{
  memset(seen, 0, sizeof seen);
  buf_len = 0;
  uthread_t t[5];
  for (int i = 0; i < 2; i++)
    REQUIRE(uthread_create(&t[i], consumer, NULL) == 0, "create");
  for (int i = 0; i < 3; i++)
    REQUIRE(uthread_create(&t[2 + i], producer, (void *)(long)i) == 0, "create");
  for (int i = 0; i < 5; i++)
    CHECK_EQ(uthread_join(t[i], NULL), 0);
  int bad = 0;
  for (int i = 0; i < 3 * ITEMS; i++)
    bad += seen[i] != 1;
  CHECK(bad == 0, "%d item(s) consumed zero or several times", bad);
  CHECK_EQ(uthread_live_stacks(), 0);
}

/* ------------------------- deadlock detection ------------------------- */
static uthread_mutex_t m1 = UTHREAD_MUTEX_INITIALIZER, m2 = UTHREAD_MUTEX_INITIALIZER;

static void *lock_12(void *arg)
{
  uthread_mutex_lock(&m1);
  uthread_yield();
  uthread_mutex_lock(&m2);
  return NULL;
}

static void *lock_21(void *arg)
{
  uthread_mutex_lock(&m2);
  uthread_yield();
  uthread_mutex_lock(&m1);
  return NULL;
}

static void classic_deadlock(void)
{
  uthread_t a, b;
  uthread_create(&a, lock_12, NULL);
  uthread_create(&b, lock_21, NULL);
  uthread_join(a, NULL);                      /* everybody is blocked now */
  printf("join returned?!\n");
}

static void *wait_forever(void *arg)
{
  uthread_mutex_lock(&cm);
  uthread_cond_wait(&cv, &cm);                /* nobody will ever signal */
  return NULL;
}

static void lost_signal(void)
{
  uthread_t t;
  uthread_create(&t, wait_forever, NULL);
  uthread_join(t, NULL);
  printf("join returned?!\n");
}

static void test_deadlock_detection(void)
{
  char out[512];
  int st = in_child(classic_deadlock, out, sizeof out);
  CHECK(WIFEXITED(st) && WEXITSTATUS(st) == UTHREAD_DEADLOCK_EXIT,
        "two threads locking in opposite orders: expected exit status %d, got %s",
        UTHREAD_DEADLOCK_EXIT, describe(st));
  CHECK(strstr(out, "uthread: deadlock") != NULL, "no \"uthread: deadlock\" message, "
        "output: \"%s\"", out);
  st = in_child(lost_signal, out, sizeof out);
  CHECK(WIFEXITED(st) && WEXITSTATUS(st) == UTHREAD_DEADLOCK_EXIT,
        "waiting for a signal that never comes: expected exit status %d, got %s",
        UTHREAD_DEADLOCK_EXIT, describe(st));
}

int main(void)
{
  RUN(test_mutex_errors);
  RUN(test_mutual_exclusion);
  RUN(test_fifo_handoff);
  RUN(test_cond_signal_order);
  RUN(test_cond_broadcast);
  RUN(test_bounded_buffer);
  RUN(test_deadlock_detection);
  return test_summary();
}
