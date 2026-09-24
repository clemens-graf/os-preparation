#include "check.h"
#include "sync.h"

#include <stdatomic.h>
#include <stdint.h>

static double cpu_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
  return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
}

static void test_trywait_counts(void)
{
  msem_t s;
  CHECK_EQ(msem_init(&s, 3), 0);
  CHECK_EQ(msem_trywait(&s), 0);
  CHECK_EQ(msem_trywait(&s), 0);
  CHECK_EQ(msem_trywait(&s), 0);
  CHECK_EQ(msem_trywait(&s), -1);
  msem_post(&s);
  CHECK_EQ(msem_trywait(&s), 0);
  CHECK_EQ(msem_trywait(&s), -1);
  msem_destroy(&s);
}

static msem_t shared;
static atomic_int passed;

static void *waits_once(void *arg)
{
  (void)arg;
  msem_wait(&shared);
  atomic_store(&passed, 1);
  return NULL;
}

static void test_wait_blocks_until_post(void)
{
  msem_init(&shared, 0);
  atomic_store(&passed, 0);
  pthread_t t;
  pthread_create(&t, NULL, waits_once, NULL);
  sleep_ms(100);
  CHECK(!atomic_load(&passed), "msem_wait returned although the value was 0");
  double before = cpu_ms();
  sleep_ms(200);
  double used = cpu_ms() - before;
  CHECK(used < 60, "a blocked msem_wait used %.0f ms CPU in 200 ms: it busy-waits", used);
  msem_post(&shared);
  long long until = now_ms() + 2000;
  while (!atomic_load(&passed) && now_ms() < until)
    sleep_ms(1);
  CHECK(atomic_load(&passed), "msem_post did not wake the waiter");
  pthread_join(t, NULL);   /* if the waiter is stuck, the program timeout hits */
  msem_destroy(&shared);
}

/* producers and consumers exchanging tokens through the semaphore */
#define PAIRS 4
#define TOKENS 5000

static void *poster(void *arg)
{
  (void)arg;
  for (int i = 0; i < TOKENS; i++)
    msem_post(&shared);
  return NULL;
}

static void *waiter(void *arg)
{
  (void)arg;
  for (int i = 0; i < TOKENS; i++)
    msem_wait(&shared);
  return NULL;
}

static void *token_exchange(void *unused)
{
  (void)unused;
  msem_init(&shared, 0);
  pthread_t p[PAIRS], w[PAIRS];
  for (int i = 0; i < PAIRS; i++)
    pthread_create(&w[i], NULL, waiter, NULL);
  for (int i = 0; i < PAIRS; i++)
    pthread_create(&p[i], NULL, poster, NULL);
  for (int i = 0; i < PAIRS; i++) {
    pthread_join(p[i], NULL);
    pthread_join(w[i], NULL);
  }
  CHECK(msem_trywait(&shared) == -1, "value should be 0 after equal numbers of posts and waits");
  msem_destroy(&shared);
  return NULL;
}

static void test_many_posts_and_waits(void)
{
  must_finish_within(60, "4 posters + 4 waiters, 5000 tokens each", token_exchange, NULL);
}

/* the semaphore as a mutex (initial value 1) */
static long counter;
static atomic_int inside;
static atomic_long violations;

static void *binary_user(void *arg)
{
  (void)arg;
  for (int i = 0; i < 5000; i++) {
    msem_wait(&shared);
    if (atomic_fetch_add(&inside, 1) != 0)
      atomic_fetch_add(&violations, 1);
    counter++;
    atomic_fetch_sub(&inside, 1);
    msem_post(&shared);
  }
  return NULL;
}

static void *binary_semaphore(void *unused)
{
  (void)unused;
  msem_init(&shared, 1);
  pthread_t t[6];
  for (int i = 0; i < 6; i++)
    pthread_create(&t[i], NULL, binary_user, NULL);
  for (int i = 0; i < 6; i++)
    pthread_join(t[i], NULL);
  CHECK(atomic_load(&violations) == 0, "a semaphore with value 1 let %ld threads in together",
        atomic_load(&violations));
  CHECK_EQ(counter, 6 * 5000);
  msem_destroy(&shared);
  return NULL;
}

static void test_binary_semaphore(void)
{
  must_finish_within(60, "semaphore used as a mutex", binary_semaphore, NULL);
}

int main(void)
{
  RUN_TEST(test_trywait_counts);
  RUN_TEST(test_wait_blocks_until_post);
  RUN_TEST(test_many_posts_and_waits);
  RUN_TEST(test_binary_semaphore);
  return test_summary();
}
