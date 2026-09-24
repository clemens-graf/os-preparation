#include "check.h"
#include "vm.h"

#include <stdint.h>

#define ALL_FREE (NFRAMES - 1)

/* A typical process layout:
 *   CODE   0x400000, 4 pages, read-only
 *   DATA   0x600000, 16 pages, writable
 *   (gap)
 *   STACK  the top 8 pages below USER_BREAK, writable; the page below is a guard
 */
#define CODE     0x400000UL
#define DATA     0x600000UL
#define STACK    (USER_BREAK - 8 * PAGE_SIZE)
#define PG(n)    ((uint64_t)(n) * PAGE_SIZE)

#define CHECK_NO_STALE_TLB()                                                    \
  CHECK(machine_tlb_stale == 0, "%zu access(es) through a stale TLB entry",     \
        machine_tlb_stale)

static struct addrspace *process(void)
{
  struct addrspace *as = as_create();
  if (!as)
    return NULL;
  if (as_add_region(as, CODE, PG(4), 0) || as_add_region(as, DATA, PG(16), 1) ||
      as_add_region(as, STACK, PG(8), 1)) {
    as_destroy(as);
    return NULL;
  }
  return as;
}

static int get(struct addrspace *as, uint64_t va)        /* byte value, or -1 */
{
  uint8_t v;
  return cpu_load(as, va, &v) == 0 ? v : -1;
}

static size_t frame_of(struct addrspace *as, uint64_t va)
{
  uint64_t *pte = vm_walk(as, va, 0);
  return pte && (*pte & PTE_P) ? PTE_PPN(*pte) : 0;
}

static uint64_t pte_of(struct addrspace *as, uint64_t va)
{
  uint64_t *pte = vm_walk(as, va, 0);
  return pte ? *pte : 0;
}

/* ------------------------------ regions ------------------------------ */
static void test_add_region(void)
{
  machine_reset();
  struct addrspace *as = as_create();
  REQUIRE(as != NULL, "as_create returned NULL");
  CHECK_EQ(as_add_region(as, DATA, PG(4), 1), 0);
  CHECK_EQ(as_add_region(as, DATA + PG(4), PG(4), 0), 0);        /* adjacent is fine */
  CHECK_EQ(as_add_region(as, DATA + PG(2), PG(4), 1), -EINVAL);  /* overlaps both */
  CHECK_EQ(as_add_region(as, DATA - PG(1), PG(2), 1), -EINVAL);  /* overlaps the first */
  CHECK_EQ(as_add_region(as, 0, PG(4), 1), -EINVAL);             /* page 0 */
  CHECK_EQ(as_add_region(as, 0x1000, 0, 1), -EINVAL);            /* empty */
  CHECK_EQ(as_add_region(as, 0x1800, PG(1), 1), -EINVAL);        /* unaligned */
  CHECK_EQ(as_add_region(as, 0x1000, 0x800, 1), -EINVAL);
  CHECK_EQ(as_add_region(as, USER_BREAK - PG(1), PG(2), 1), -EINVAL);
  CHECK_EQ(as_add_region(as, USER_BREAK - PG(1), PG(1), 1), 0);  /* ends exactly there */
  CHECK_EQ(as_add_region(as, 0x10000, 0 - 0x8000UL, 1), -EINVAL);/* start + len wraps */
  CHECK_EQ(as->nregions, 3);
  for (int i = 3; i < MAX_REGIONS; i++)
    CHECK_EQ(as_add_region(as, PG(100 + 10 * i), PG(1), 0), 0);
  CHECK_EQ(as_add_region(as, PG(500), PG(1), 0), -ENOMEM);
  CHECK(frames_free() == ALL_FREE - 1, "adding regions must not allocate frames");
  as_destroy(as);
}

/* ------------------------- lazy allocation ------------------------- */
static void test_lazy_allocation(void)
{
  machine_reset();
  struct addrspace *as = process();
  REQUIRE(as != NULL, "could not set up the process");
  size_t before = frames_free();

  CHECK_EQ(cpu_store(as, DATA + PG(5) + 7, 42), 0);
  CHECK_EQ(machine_page_faults, 1);
  CHECK(before - frames_free() == 4, "first touch should cost 3 tables + 1 page, cost %zu",
        before - frames_free());
  CHECK_EQ(get(as, DATA + PG(5) + 7), 42);
  CHECK_EQ(cpu_store(as, DATA + PG(5) + 8, 43), 0);
  CHECK_EQ(machine_page_faults, 1);                      /* no more faults */

  CHECK_EQ(get(as, DATA + PG(1) + 100), 0);              /* a fresh page reads zero */
  CHECK_EQ(machine_page_faults, 2);
  size_t f = frame_of(as, DATA + PG(1));
  REQUIRE(f != 0, "the page is not mapped after the fault");
  const unsigned char *p = phys(f);
  size_t nonzero = 0;
  for (size_t i = 0; i < PAGE_SIZE; i++)
    nonzero += p[i] != 0;
  CHECK(nonzero == 0, "the new page has %zu non-zero bytes - old contents leaked", nonzero);

  CHECK_EQ(get(as, STACK + PG(8) - 1), 0);               /* the very top of the stack */
  CHECK_EQ(cpu_store(as, STACK, 1), 0);                  /* its lowest page */
  as_destroy(as);
  CHECK_EQ(frames_free(), ALL_FREE);
}

static void test_no_leak_between_processes(void)
{
  machine_reset();
  struct addrspace *a = process();
  REQUIRE(a != NULL, "could not set up the process");
  const char secret[] = "password=hunter2";
  for (int pg = 0; pg < 4; pg++)
    for (size_t i = 0; i < sizeof secret; i++)
      cpu_store(a, DATA + PG(pg) + i, (uint8_t)secret[i]);
  as_destroy(a);                         /* the frames go back, secret and all */

  struct addrspace *b = process();
  REQUIRE(b != NULL, "could not set up the process");
  int leaked = 0;
  for (int pg = 0; pg < 4; pg++)
    for (size_t i = 0; i < sizeof secret; i++)
      leaked += get(b, DATA + PG(pg) + i) != 0;
  CHECK(leaked == 0, "process b sees %d non-zero bytes of process a's memory", leaked);
  as_destroy(b);
}

static void test_segfaults(void)
{
  machine_reset();
  struct addrspace *as = process();
  REQUIRE(as != NULL, "could not set up the process");
  CHECK_EQ(get(as, 0), -1);                              /* NULL */
  CHECK_EQ(get(as, 0x10), -1);
  CHECK_EQ(get(as, DATA + PG(16)), -1);                  /* just past DATA */
  CHECK_EQ(get(as, CODE - 1), -1);
  CHECK_EQ(get(as, STACK - 1), -1);                      /* the guard page */
  CHECK_EQ(get(as, USER_BREAK + 5), -1);                 /* kernel */

  CHECK_EQ(get(as, CODE + 10), 0);                       /* read-only: readable */
  CHECK_EQ(cpu_store(as, CODE + 10, 1), -1);             /* ... but not writable */
  CHECK_EQ(cpu_store(as, CODE + PG(2), 1), -1);          /* not even when not yet present */
  CHECK(!(pte_of(as, CODE) & PTE_W), "a page of a read-only region is mapped writable");
  as_destroy(as);
  CHECK_EQ(frames_free(), ALL_FREE);
}

/* ------------------------------ fork ------------------------------ */
static void test_fork_shares_frames(void)
{
  machine_reset();
  struct addrspace *parent = process();
  REQUIRE(parent != NULL, "could not set up the process");
  cpu_store(parent, DATA, 'P');
  cpu_store(parent, DATA + PG(1), 'Q');
  get(parent, CODE);
  size_t data0 = frame_of(parent, DATA), code0 = frame_of(parent, CODE);

  size_t before = frames_free();
  struct addrspace *child = as_fork(parent);
  REQUIRE(child != NULL, "as_fork returned NULL");
  CHECK(before - frames_free() == 5, "fork should only allocate the child's page tables "
        "(PML4, PDPT, PD, 2 PTs), it allocated %zu frames", before - frames_free());
  CHECK_EQ(child->nregions, parent->nregions);

  size_t faults = machine_page_faults;
  CHECK_EQ(get(child, DATA), 'P');
  CHECK_EQ(get(child, DATA + PG(1)), 'Q');
  CHECK_EQ(machine_page_faults, faults);                 /* already mapped: no faults */
  CHECK_EQ(frame_of(child, DATA), data0);
  CHECK_EQ(frame_of(child, CODE), code0);
  CHECK_EQ(frame_refcount(data0), 2);
  CHECK_EQ(frame_refcount(code0), 2);

  uint64_t pp = pte_of(parent, DATA), pc = pte_of(child, DATA);
  CHECK((pp & (PTE_W | PTE_COW)) == PTE_COW, "parent's data page: expected read-only + COW");
  CHECK((pc & (PTE_W | PTE_COW)) == PTE_COW, "child's data page: expected read-only + COW");
  CHECK(!(pte_of(child, CODE) & (PTE_W | PTE_COW)), "code pages are shared without COW");
  as_destroy(child);
  as_destroy(parent);
  CHECK_EQ(frames_free(), ALL_FREE);
}

static void test_cow_write(void)
{
  machine_reset();
  struct addrspace *parent = process();
  REQUIRE(parent != NULL, "could not set up the process");
  cpu_store(parent, DATA + 5, 'P');
  size_t data0 = frame_of(parent, DATA);
  struct addrspace *child = as_fork(parent);
  REQUIRE(child != NULL, "as_fork returned NULL");

  size_t faults = machine_page_faults;
  CHECK_EQ(cpu_store(child, DATA + 5, 'c'), 0);
  CHECK_EQ(machine_page_faults, faults + 1);
  CHECK_EQ(get(child, DATA + 5), 'c');
  CHECK_EQ(get(parent, DATA + 5), 'P');                  /* the parent is unaffected */
  CHECK(frame_of(child, DATA) != data0, "the child still uses the shared frame");
  CHECK_EQ(frame_refcount(data0), 1);
  CHECK(pte_of(child, DATA) & PTE_W, "the child's copy must be writable");

  /* the parent is now the only user: no copy, just writable again */
  CHECK_EQ(cpu_store(parent, DATA + 5, 'p'), 0);
  CHECK_EQ(frame_of(parent, DATA), data0);
  CHECK((pte_of(parent, DATA) & (PTE_W | PTE_COW)) == PTE_W, "expected writable, no COW");
  CHECK_EQ(get(child, DATA + 5), 'c');
  CHECK_NO_STALE_TLB();
  as_destroy(parent);
  as_destroy(child);
  CHECK_EQ(frames_free(), ALL_FREE);
}

static void test_cow_parent_tlb(void)
{
  machine_reset();
  struct addrspace *parent = process();
  REQUIRE(parent != NULL, "could not set up the process");
  cpu_store(parent, DATA, 'A');          /* the TLB now says: DATA is writable */
  struct addrspace *child = as_fork(parent);
  REQUIRE(child != NULL, "as_fork returned NULL");
  CHECK_EQ(cpu_store(parent, DATA, 'B'), 0);             /* must copy, not write through */
  CHECK_EQ(get(child, DATA), 'A');
  CHECK_EQ(get(parent, DATA), 'B');
  CHECK_NO_STALE_TLB();
  as_destroy(child);
  as_destroy(parent);
}

static void test_fork_untouched_and_readonly(void)
{
  machine_reset();
  struct addrspace *parent = process();
  REQUIRE(parent != NULL, "could not set up the process");
  get(parent, CODE);
  struct addrspace *child = as_fork(parent);
  REQUIRE(child != NULL, "as_fork returned NULL");

  CHECK_EQ(cpu_store(child, DATA + PG(3), 'c'), 0);      /* never touched before fork */
  CHECK_EQ(get(parent, DATA + PG(3)), 0);
  CHECK(frame_of(child, DATA + PG(3)) != frame_of(parent, DATA + PG(3)),
        "lazily allocated pages must not be shared");

  CHECK_EQ(cpu_store(child, CODE, 1), -1);               /* read-only stays read-only */
  CHECK_EQ(cpu_store(parent, CODE, 1), -1);
  as_destroy(parent);
  as_destroy(child);
  CHECK_EQ(frames_free(), ALL_FREE);
}

static void test_fork_chain(void)
{
  machine_reset();
  struct addrspace *a = process();
  REQUIRE(a != NULL, "could not set up the process");
  for (int pg = 0; pg < 4; pg++)
    cpu_store(a, DATA + PG(pg), (uint8_t)('a' + pg));
  struct addrspace *b = as_fork(a);
  REQUIRE(b != NULL, "as_fork returned NULL");
  cpu_store(b, DATA + PG(1), 'B');
  struct addrspace *c = as_fork(b);
  REQUIRE(c != NULL, "as_fork returned NULL");
  CHECK_EQ(frame_refcount(frame_of(a, DATA)), 3);        /* page 0: shared by all three */
  cpu_store(c, DATA + PG(2), 'C');

  as_destroy(a);                                         /* the original goes first */
  CHECK_EQ(get(b, DATA), 'a');
  CHECK_EQ(get(c, DATA), 'a');
  CHECK_EQ(get(b, DATA + PG(1)), 'B');
  CHECK_EQ(get(c, DATA + PG(1)), 'B');
  CHECK_EQ(get(b, DATA + PG(2)), 'c');
  CHECK_EQ(get(c, DATA + PG(2)), 'C');
  CHECK_EQ(cpu_store(c, DATA, 'z'), 0);
  CHECK_EQ(get(b, DATA), 'a');
  as_destroy(c);
  CHECK_EQ(get(b, DATA + PG(3)), 'd');
  as_destroy(b);
  CHECK(frames_free() == ALL_FREE, "%zu frame(s) leaked", ALL_FREE - frames_free());
  CHECK_NO_STALE_TLB();
}

static void test_many_forks(void)
{
  machine_reset();
  struct addrspace *parent = process();
  REQUIRE(parent != NULL, "could not set up the process");
  for (int pg = 0; pg < 16; pg++)
    cpu_store(parent, DATA + PG(pg), (uint8_t)pg);
  size_t baseline = frames_free();
  for (int round = 0; round < 25; round++) {
    struct addrspace *child = as_fork(parent);
    REQUIRE(child != NULL, "as_fork returned NULL in round %d", round);
    for (int pg = 0; pg < 16; pg++)
      cpu_store(child, DATA + PG(pg), (uint8_t)(100 + round));
    if (round % 2)
      cpu_store(parent, DATA + PG(round % 16), (uint8_t)(round % 16));
    as_destroy(child);
    CHECK(frames_free() == baseline, "round %d: %zd frame(s) leaked", round,
          (ssize_t)(baseline - frames_free()));
  }
  for (int pg = 0; pg < 16; pg++)
    CHECK(get(parent, DATA + PG(pg)) == pg, "parent's page %d changed", pg);
  CHECK_NO_STALE_TLB();
  as_destroy(parent);
}

int main(void)
{
  RUN_TEST(test_add_region);
  RUN_TEST(test_lazy_allocation);
  RUN_TEST(test_no_leak_between_processes);
  RUN_TEST(test_segfaults);
  RUN_TEST(test_fork_shares_frames);
  RUN_TEST(test_cow_write);
  RUN_TEST(test_cow_parent_tlb);
  RUN_TEST(test_fork_untouched_and_readonly);
  RUN_TEST(test_fork_chain);
  RUN_TEST(test_many_forks);
  return test_summary();
}
