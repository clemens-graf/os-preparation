#include "check.h"
#include "sync.h"

#include <stdatomic.h>
#include <stdint.h>

static rwlock_t rw;
static atomic_int readers_inside, writers_inside;
static atomic_long violations;
static long shared_value;          /* written only by writers, under the lock */

static void *reader_loop(void *arg)
{
  (void)arg;
  for (int i = 0; i < 3000; i++) {
    rw_read_lock(&rw);
    atomic_fetch_add(&readers_inside, 1);
    if (atomic_load(&writers_inside) != 0)
      atomic_fetch_add(&violations, 1);
    long v = shared_value;         /* reading is fine while other readers read */
    (void)v;
    atomic_fetch_sub(&readers_inside, 1);
    rw_read_unlock(&rw);
  }
  return NULL;
}

static void *writer_loop(void *arg)
{
  (void)arg;
  for (int i = 0; i < 1000; i++) {
    rw_write_lock(&rw);
    if (atomic_fetch_add(&writers_inside, 1) != 0 || atomic_load(&readers_inside) != 0)
      atomic_fetch_add(&violations, 1);
    shared_value++;
    atomic_fetch_sub(&writers_inside, 1);
    rw_write_unlock(&rw);
  }
  return NULL;
}

static void *exclusion(void *unused)
{
  (void)unused;
  CHECK_EQ(rw_init(&rw), 0);
  shared_value = 0;
  atomic_store(&violations, 0);
  pthread_t r[6], w[2];
  for (int i = 0; i < 6; i++)
    pthread_create(&r[i], NULL, reader_loop, NULL);
  for (int i = 0; i < 2; i++)
    pthread_create(&w[i], NULL, writer_loop, NULL);
  for (int i = 0; i < 6; i++)
    pthread_join(r[i], NULL);
  for (int i = 0; i < 2; i++)
    pthread_join(w[i], NULL);
  CHECK(atomic_load(&violations) == 0,
        "%ld time(s) a writer was inside together with a reader or another writer",
        atomic_load(&violations));
  CHECK_EQ(shared_value, 2000);
  rw_destroy(&rw);
  return NULL;
}

static void test_exclusion(void)
{
  must_finish_within(60, "6 readers + 2 writers", exclusion, NULL);
}

/* Four readers must be able to hold the lock AT THE SAME TIME: each waits
 * at a barrier inside the read section, which only opens when all four
 * are inside. A lock that serialises readers deadlocks here. */
static pthread_barrier_t inside_together;

static void *barrier_reader(void *arg)
{
  (void)arg;
  rw_read_lock(&rw);
  pthread_barrier_wait(&inside_together);
  rw_read_unlock(&rw);
  return NULL;
}

static void *readers_share(void *unused)
{
  (void)unused;
  rw_init(&rw);
  pthread_barrier_init(&inside_together, NULL, 4);
  pthread_t t[4];
  for (int i = 0; i < 4; i++)
    pthread_create(&t[i], NULL, barrier_reader, NULL);
  for (int i = 0; i < 4; i++)
    pthread_join(t[i], NULL);
  pthread_barrier_destroy(&inside_together);
  rw_destroy(&rw);
  return NULL;
}

static void test_readers_share(void)
{
  must_finish_within(10, "4 readers holding the read lock simultaneously", readers_share, NULL);
}

/* Writer preference: R1 reads; W starts waiting; R2 arrives and must wait
 * behind W. Expected order of acquisition: W before R2. */
static atomic_int step;
static int order[2];
static atomic_int order_idx;

static void *late_writer(void *arg)
{
  (void)arg;
  rw_write_lock(&rw);
  order[atomic_fetch_add(&order_idx, 1)] = 'W';
  rw_write_unlock(&rw);
  return NULL;
}

static void *late_reader(void *arg)
{
  (void)arg;
  rw_read_lock(&rw);
  order[atomic_fetch_add(&order_idx, 1)] = 'R';
  rw_read_unlock(&rw);
  return NULL;
}

static void *writer_preference(void *unused)
{
  (void)unused;
  rw_init(&rw);
  atomic_store(&order_idx, 0);
  rw_read_lock(&rw);                        /* R1 = main */
  pthread_t w, r2;
  pthread_create(&w, NULL, late_writer, NULL);
  sleep_ms(100);                            /* W is now waiting */
  pthread_create(&r2, NULL, late_reader, NULL);
  sleep_ms(100);                            /* R2 has arrived */
  CHECK(atomic_load(&order_idx) == 0, "someone got the lock while R1 was reading and W was waiting "
        "(first: %c) - new readers must wait behind a waiting writer", order[0]);
  rw_read_unlock(&rw);
  pthread_join(w, NULL);
  pthread_join(r2, NULL);
  CHECK(order[0] == 'W' && order[1] == 'R', "acquisition order %c%c, expected WR", order[0], order[1]);
  rw_destroy(&rw);
  return NULL;
}

static void test_writer_preference(void)
{
  must_finish_within(10, "writer preference", writer_preference, NULL);
}

int main(void)
{
  RUN_TEST(test_readers_share);
  RUN_TEST(test_exclusion);
  RUN_TEST(test_writer_preference);
  return test_summary();
}
