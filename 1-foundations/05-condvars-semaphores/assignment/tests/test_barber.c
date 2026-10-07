#include "check.h"
#include "sync.h"

#include <stdatomic.h>
#include <stdint.h>

static shop_t shop;
static atomic_int cuts_started, cuts_done, in_chair;
static atomic_int served, turned_away, early_returns, overlapping_cuts;
static int cut_ms;

static void cut_hair(void *ctx)
{
  (void)ctx;
  if (atomic_fetch_add(&in_chair, 1) != 0)
    atomic_fetch_add(&overlapping_cuts, 1);
  atomic_fetch_add(&cuts_started, 1);
  sleep_ms(cut_ms);
  atomic_fetch_sub(&in_chair, 1);
  atomic_fetch_add(&cuts_done, 1);
}

static void *customer(void *arg)
{
  (void)arg;
  if (shop_visit(&shop)) {
    /* my haircut must be complete: at least as many completed cuts as
     * customers that have left with a haircut (including me) */
    int me = atomic_fetch_add(&served, 1) + 1;
    if (atomic_load(&cuts_done) < me)
      atomic_fetch_add(&early_returns, 1);
  } else {
    atomic_fetch_add(&turned_away, 1);
  }
  return NULL;
}

static atomic_int barber_returned;

static void *barber(void *arg)
{
  (void)arg;
  barber_run(&shop);
  atomic_store(&barber_returned, 1);
  return NULL;
}

struct scenario { unsigned chairs; int customers; int cut_ms; int gap_ms; };

static void reset(int ms)
{
  cut_ms = ms;
  atomic_store(&cuts_started, 0);
  atomic_store(&cuts_done, 0);
  atomic_store(&in_chair, 0);
  atomic_store(&served, 0);
  atomic_store(&turned_away, 0);
  atomic_store(&early_returns, 0);
  atomic_store(&overlapping_cuts, 0);
}

static void *run_scenario(void *p)
{
  struct scenario *sc = p;
  reset(sc->cut_ms);
  CHECK_EQ(shop_init(&shop, sc->chairs, cut_hair, NULL), 0);
  pthread_t b, c[64];
  pthread_create(&b, NULL, barber, NULL);
  for (int i = 0; i < sc->customers; i++) {
    pthread_create(&c[i], NULL, customer, NULL);
    if (sc->gap_ms)
      sleep_ms(sc->gap_ms);
  }
  for (int i = 0; i < sc->customers; i++)
    pthread_join(c[i], NULL);
  shop_close(&shop);
  pthread_join(b, NULL);
  shop_destroy(&shop);

  int s = atomic_load(&served), t = atomic_load(&turned_away);
  CHECK(s + t == sc->customers, "%d served + %d turned away != %d customers", s, t, sc->customers);
  CHECK(atomic_load(&cuts_done) == s, "the barber cut %d times for %d served customers",
        atomic_load(&cuts_done), s);
  CHECK(atomic_load(&early_returns) == 0, "%d customer(s) left before their haircut was finished",
        atomic_load(&early_returns));
  CHECK(atomic_load(&overlapping_cuts) == 0, "two haircuts overlapped - there is only one barber chair");
  return NULL;
}

static void test_everyone_served_with_enough_chairs(void)
{
  struct scenario sc = {.chairs = 20, .customers = 20, .cut_ms = 2, .gap_ms = 0};
  must_finish_within(20, "20 customers, 20 chairs", run_scenario, &sc);
  CHECK(atomic_load(&served) == 20, "only %d of 20 served although there were enough chairs",
        atomic_load(&served));
}

static void test_full_shop_turns_customers_away(void)
{
  /* 12 customers at once, 2 chairs, slow barber: at most 1 in the chair +
   * 2 waiting can be served. */
  struct scenario sc = {.chairs = 2, .customers = 12, .cut_ms = 200, .gap_ms = 0};
  must_finish_within(20, "12 customers, 2 chairs", run_scenario, &sc);
  int s = atomic_load(&served);
  CHECK(s >= 1 && s <= 3, "served %d customers; with 2 chairs and a slow barber expected 1..3", s);
}

static void test_customers_trickle_in(void)
{
  /* Customers arrive slower than the barber works: nobody is turned away,
   * and the barber has to fall asleep and be woken again many times. */
  struct scenario sc = {.chairs = 1, .customers = 15, .cut_ms = 1, .gap_ms = 10};
  must_finish_within(20, "15 customers arriving one by one", run_scenario, &sc);
  CHECK(atomic_load(&served) == 15, "served %d of 15 (the barber did not wake up?)",
        atomic_load(&served));
}

static double cpu_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
  return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
}

static void *idle_barber(void *unused)
{
  (void)unused;
  reset(1);
  shop_init(&shop, 3, cut_hair, NULL);
  atomic_store(&barber_returned, 0);
  pthread_t b;
  pthread_create(&b, NULL, barber, NULL);
  sleep_ms(50);
  double before = cpu_ms();
  sleep_ms(250);
  double used = cpu_ms() - before;
  CHECK(used < 60, "the idle barber used %.0f ms CPU in 250 ms: he must sleep, not spin", used);
  CHECK(!atomic_load(&barber_returned), "barber_run returned before shop_close was called");
  shop_close(&shop);                      /* must wake the sleeping barber */
  pthread_join(b, NULL);
  shop_destroy(&shop);
  return NULL;
}

static void test_barber_sleeps_and_closing_wakes_him(void)
{
  must_finish_within(10, "idle barber + shop_close", idle_barber, NULL);
}

int main(void)
{
  RUN_TEST(test_everyone_served_with_enough_chairs);
  RUN_TEST(test_full_shop_turns_customers_away);
  RUN_TEST(test_customers_trickle_in);
  RUN_TEST(test_barber_sleeps_and_closing_wakes_him);
  return test_summary();
}
