#include "check.h"
#include "once.h"

#include <stdatomic.h>
#include <stdint.h>

#define CALLERS 16

static atomic_int init_calls;
static atomic_int init_finished;
static atomic_int early_returns;
static pthread_barrier_t start_line;
static my_once_t *current;

static void slow_init(void)
{
  atomic_fetch_add(&init_calls, 1);
  sleep_ms(30);                          /* a slow initialisation */
  atomic_store(&init_finished, 1);
}

static void *caller(void *arg)
{
  (void)arg;
  pthread_barrier_wait(&start_line);     /* everyone calls at the same moment */
  my_once(current, slow_init);
  if (!atomic_load(&init_finished))
    atomic_fetch_add(&early_returns, 1); /* returned before init completed! */
  return NULL;
}

/* One fresh once-object per round (copying objects that may contain a
 * mutex is not allowed, so they are all statically initialised). */
static my_once_t rounds[5] = {MY_ONCE_INIT, MY_ONCE_INIT, MY_ONCE_INIT, MY_ONCE_INIT, MY_ONCE_INIT};

static void *one_round(void *arg)
{
  current = &rounds[(intptr_t)arg];
  atomic_store(&init_calls, 0);
  atomic_store(&init_finished, 0);
  atomic_store(&early_returns, 0);

  pthread_barrier_init(&start_line, NULL, CALLERS);
  pthread_t t[CALLERS];
  for (int i = 0; i < CALLERS; i++)
    pthread_create(&t[i], NULL, caller, NULL);
  for (int i = 0; i < CALLERS; i++)
    pthread_join(t[i], NULL);
  pthread_barrier_destroy(&start_line);

  CHECK(atomic_load(&init_calls) == 1, "init ran %d times, expected exactly once",
        atomic_load(&init_calls));
  CHECK(atomic_load(&early_returns) == 0,
        "%d caller(s) returned from my_once before init had finished",
        atomic_load(&early_returns));

  /* later calls must not run init again */
  my_once(current, slow_init);
  CHECK(atomic_load(&init_calls) == 1, "a later call ran init again");
  return NULL;
}

static void test_once_concurrent(void)
{
  for (intptr_t round = 0; round < 5; round++)
    must_finish_within(20, "my_once with 16 concurrent callers", one_round, (void *)round);
}

static int sequential_calls;
static void count_init(void) { sequential_calls++; }

static void test_once_sequential(void)
{
  my_once_t o = MY_ONCE_INIT;
  for (int i = 0; i < 10; i++)
    my_once(&o, count_init);
  CHECK_EQ(sequential_calls, 1);
}

int main(void)
{
  RUN_TEST(test_once_sequential);
  RUN_TEST(test_once_concurrent);
  return test_summary();
}
