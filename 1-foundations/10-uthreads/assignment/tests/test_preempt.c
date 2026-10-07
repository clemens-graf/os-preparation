#include "helpers.h"

/*
 * With preemption, a thread can be interrupted ANYWHERE - also inside
 * printf or malloc, which hold internal locks. A second uthread calling
 * them then deadlocks (it is the same kernel thread). So the threads
 * below only compute and record; the checks happen in main afterwards.
 * (SWEB has the same rule: no new/delete in interrupt handlers.)
 */

#define TICK_US 1000

static int preemption_available(void)
{
  return uthread_preempt(TICK_US) != ENOSYS;
}

/* ---- two threads wait for each other WITHOUT yielding ---- */
static volatile int flag_a, flag_b;

static void *spin_a(void *arg)
{
  flag_a = 1;
  while (!flag_b) {}
  return NULL;
}

static void *spin_b(void *arg)
{
  flag_b = 1;
  while (!flag_a) {}
  return NULL;
}

static void test_spinners_progress(void)
{
  if (!preemption_available())
    SKIP("uthread_preempt not implemented (bonus)");
  flag_a = flag_b = 0;
  uthread_t a, b;
  REQUIRE(uthread_create(&a, spin_a, NULL) == 0, "create");
  REQUIRE(uthread_create(&b, spin_b, NULL) == 0, "create");
  CHECK_EQ(uthread_join(a, NULL), 0);          /* hangs without preemption */
  CHECK_EQ(uthread_join(b, NULL), 0);
  uthread_preempt(0);
}

/* ---- a mutex under preemption ---- */
#define ROUNDS 20000
static long counter;
static uthread_mutex_t lock = UTHREAD_MUTEX_INITIALIZER;

static void *locked_increment(void *arg)
{
  long interleaved = 0, last = -1;
  for (int i = 0; i < ROUNDS; i++) {
    uthread_mutex_lock(&lock);
    long v = counter;
    for (volatile int w = 0; w < 50; w++) {}   /* a long critical section */
    counter = v + 1;
    if (last >= 0 && v != last + 1)
      interleaved++;                           /* somebody else ran in between */
    last = v;
    uthread_mutex_unlock(&lock);
  }
  return (void *)interleaved;
}

static void test_mutex_under_preemption(void)
{
  if (!preemption_available())
    SKIP("uthread_preempt not implemented (bonus)");
  counter = 0;
  uthread_t t[4];
  for (int i = 0; i < 4; i++)
    REQUIRE(uthread_create(&t[i], locked_increment, NULL) == 0, "create");
  long interleaved = 0;
  for (int i = 0; i < 4; i++) {
    void *r;
    CHECK_EQ(uthread_join(t[i], &r), 0);
    interleaved += (long)r;
  }
  uthread_preempt(0);
  CHECK_EQ(counter, 4L * ROUNDS);
  CHECK(interleaved > 0, "the threads never interleaved - is the timer running?");
  CHECK_EQ(lock.owner, -1);
}

/* ---- the library's own data under fire ----
 * Threads that do almost nothing but call the library: most timer ticks
 * land INSIDE uthread_* functions. Unprotected, the ready queue, the
 * thread table or a mutex's waiter queue get corrupted sooner or later. */
static void *tiny(void *arg)
{
  return arg;
}

static void *spawner(void *arg)
{
  long sum = 0;
  for (int i = 0; i < 1500; i++) {
    uthread_t t;
    if (uthread_create(&t, tiny, (void *)1L) != 0)
      return (void *)-1L;
    void *r;
    if (uthread_join(t, &r) != 0)
      return (void *)-2L;
    sum += (long)r;
  }
  return (void *)sum;
}

static uthread_mutex_t busy_lock = UTHREAD_MUTEX_INITIALIZER;
static long busy_counter;

static void *locker(void *arg)
{
  for (int i = 0; i < 100000; i++) {
    if (uthread_mutex_lock(&busy_lock) != 0)
      return (void *)-1L;
    busy_counter++;
    if (uthread_mutex_unlock(&busy_lock) != 0)
      return (void *)-2L;
    if (i % 64 == 0)
      uthread_yield();
  }
  return NULL;
}

static void test_library_is_interrupt_safe(void)
{
  if (!preemption_available())
    SKIP("uthread_preempt not implemented (bonus)");
  busy_counter = 0;
  uthread_t s[3], l[3];
  for (int i = 0; i < 3; i++) {
    REQUIRE(uthread_create(&s[i], spawner, NULL) == 0, "create");
    REQUIRE(uthread_create(&l[i], locker, NULL) == 0, "create");
  }
  for (int i = 0; i < 3; i++) {
    void *r;
    CHECK_EQ(uthread_join(s[i], &r), 0);
    CHECK((long)r == 1500, "spawner %d: %ld (negative: create/join failed)", i, (long)r);
    CHECK_EQ(uthread_join(l[i], &r), 0);
    CHECK(r == NULL, "locker %d: lock/unlock failed (%ld)", i, (long)r);
  }
  uthread_preempt(0);
  CHECK_EQ(busy_counter, 300000);
  CHECK_EQ(uthread_live_stacks(), 0);
}

int main(void)
{
  RUN(test_spinners_progress);
  RUN(test_mutex_under_preemption);
  RUN(test_library_is_interrupt_safe);
  return test_summary();
}
