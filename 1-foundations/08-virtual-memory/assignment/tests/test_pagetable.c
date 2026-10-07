#include "check.h"
#include "vm.h"

#include <stdint.h>

#define ALL_FREE (NFRAMES - 1)

#define CHECK_NO_STALE_TLB()                                                    \
  CHECK(machine_tlb_stale == 0, "%zu access(es) through a stale TLB entry",     \
        machine_tlb_stale)

/* A frame filled with `fill`, owned by the caller. */
static size_t data_frame(unsigned char fill)
{
  size_t f = frame_alloc();
  memset(phys(f), fill, PAGE_SIZE);
  return f;
}

static int load(struct addrspace *as, uint64_t va, uint8_t *v)
{
  return cpu_load(as, va, v);
}

/* ------------------------------------------------------------------ */
static void test_create_destroy(void)
{
  machine_reset();
  struct addrspace *as = as_create();
  REQUIRE(as != NULL, "as_create returned NULL");
  CHECK_EQ(frames_free(), ALL_FREE - 1);
  CHECK(as->root != 0, "no PML4");
  const uint64_t *pml4 = phys(as->root);
  int dirty = 0;
  for (int i = 0; i < 512; i++)
    dirty += pml4[i] != 0;
  CHECK(dirty == 0, "the new PML4 has %d non-zero entries (frames come full of junk)", dirty);
  CHECK(vm_walk(as, 0x400000, 0) == NULL, "vm_walk without create on an empty space");
  as_destroy(as);
  CHECK_EQ(frames_free(), ALL_FREE);
}

static void test_map_and_access(void)
{
  machine_reset();
  struct addrspace *as = as_create();
  REQUIRE(as != NULL, "as_create returned NULL");
  size_t f = data_frame('x');
  size_t before = frames_free();

  REQUIRE(vm_map(as, 0x400000, f, PTE_W) == 0, "vm_map failed");
  CHECK_EQ(before - frames_free(), 3);                  /* PDPT, PD, PT */
  uint64_t *pte = vm_walk(as, 0x400000, 0);
  REQUIRE(pte != NULL, "vm_walk finds no entry for a mapped page");
  CHECK(*pte == PTE_MAKE(f, PTE_P | PTE_U | PTE_W), "leaf entry is %#llx, expected %#llx",
        (unsigned long long)*pte, (unsigned long long)PTE_MAKE(f, PTE_P | PTE_U | PTE_W));

  uint8_t v = 0;
  CHECK_EQ(load(as, 0x400123, &v), 0);
  CHECK_EQ(v, 'x');
  CHECK_EQ(cpu_store(as, 0x400abc, 'y'), 0);
  CHECK_EQ(((unsigned char *)phys(f))[0xabc], 'y');

  /* how many new tables does each further mapping need? */
  static const struct { uint64_t va; int tables; const char *why; } more[] = {
    {0x401000, 0, "same page table"},
    {0x600000, 1, "next 2 MiB: new PT"},
    {0x40000000, 2, "next 1 GiB: new PD + PT"},
    {0x8000000000, 3, "next 512 GiB: new PDPT + PD + PT"},
    {USER_BREAK - PAGE_SIZE, 3, "last user page"},
  };
  for (size_t i = 0; i < sizeof more / sizeof more[0]; i++) {
    size_t g = data_frame((unsigned char)('a' + i));
    before = frames_free();
    CHECK(vm_map(as, more[i].va, g, 0) == 0, "vm_map(%#llx) failed", (unsigned long long)more[i].va);
    CHECK(before - frames_free() == (size_t)more[i].tables, "%s: %zu new table(s), expected %d",
          more[i].why, before - frames_free(), more[i].tables);
    CHECK(load(as, more[i].va + 7, &v) == 0 && v == 'a' + i, "cannot read the page at %#llx",
          (unsigned long long)more[i].va);
  }
  as_destroy(as);
  CHECK_EQ(frames_free(), ALL_FREE);
  CHECK_NO_STALE_TLB();
}

static void test_intermediate_entries(void)
{
  machine_reset();
  struct addrspace *as = as_create();
  REQUIRE(as != NULL, "as_create returned NULL");
  REQUIRE(vm_map(as, 0x7f0000200000, data_frame(0), PTE_W) == 0, "vm_map failed");
  size_t t = as->root;
  for (int level = 3; level >= 1; level--) {
    uint64_t e = ((uint64_t *)phys(t))[PTE_INDEX(0x7f0000200000, level)];
    CHECK((e & (PTE_P | PTE_W | PTE_U)) == (PTE_P | PTE_W | PTE_U),
          "level-%d entry %#llx lacks P, W or U - the MMU ANDs all levels", level,
          (unsigned long long)e);
    t = PTE_PPN(e);
  }
  CHECK_EQ(cpu_store(as, 0x7f0000200010, 1), 0);
  as_destroy(as);
}

static void test_protection(void)
{
  machine_reset();
  struct addrspace *as = as_create();
  REQUIRE(as != NULL, "as_create returned NULL");
  REQUIRE(vm_map(as, 0x400000, data_frame('r'), 0) == 0, "vm_map failed");  /* read-only */
  uint8_t v;
  CHECK(load(as, 0x400000, &v) == 0 && v == 'r', "cannot read a read-only page");
  CHECK_EQ(cpu_store(as, 0x400000, 'w'), -1);           /* no regions: page_fault says no */
  CHECK(load(as, 0x400000, &v) == 0 && v == 'r', "the read-only page was modified");

  CHECK_EQ(load(as, 0x401000, &v), -1);                 /* same PT, not mapped */
  CHECK_EQ(load(as, 0x7000000000, &v), -1);             /* no tables at all */
  CHECK_EQ(load(as, 0, &v), -1);
  CHECK_EQ(load(as, USER_BREAK + 0x1000, &v), -1);
  as_destroy(as);
}

static void test_map_errors(void)
{
  machine_reset();
  struct addrspace *as = as_create();
  REQUIRE(as != NULL, "as_create returned NULL");
  size_t f = data_frame('1'), g = data_frame('2');
  CHECK_EQ(vm_map(as, 0x400010, f, 0), -EINVAL);
  CHECK_EQ(vm_map(as, USER_BREAK, f, 0), -EINVAL);
  CHECK_EQ(vm_map(as, 0xffff800000000000, f, 0), -EINVAL);
  CHECK_EQ(vm_map(as, 0x400000, f, 0), 0);
  CHECK_EQ(vm_map(as, 0x400000, g, PTE_W), -EEXIST);
  uint8_t v;
  CHECK(load(as, 0x400000, &v) == 0 && v == '1', "-EEXIST must keep the old mapping");
  CHECK_EQ(frame_refcount(g), 1);                       /* still ours */
  frame_unref(g);
  as_destroy(as);
  CHECK_EQ(frames_free(), ALL_FREE);
}

static void test_map_out_of_memory(void)
{
  machine_reset();
  struct addrspace *as = as_create();
  REQUIRE(as != NULL, "as_create returned NULL");
  REQUIRE(vm_map(as, 0x400000, data_frame(0), PTE_W) == 0, "vm_map failed");

  static size_t held[NFRAMES];
  int n = 0;
  while (frames_free() > 2)                     /* leave exactly 2 frames */
    held[n++] = frame_alloc();

  /* 512 GiB away needs 3 new tables, only 2 are there */
  CHECK_EQ(vm_map(as, 0x8000000000, held[0], PTE_W), -ENOMEM);
  CHECK(frames_free() == 2, "a failed vm_map left %zu page table(s) behind", 2 - frames_free());
  CHECK(vm_walk(as, 0x8000000000, 0) == NULL, "half-built tables after -ENOMEM");

  CHECK_EQ(vm_map(as, 0x600000, held[0], PTE_W), 0);   /* needs 1 table: fits */
  CHECK_EQ(frames_free(), 1);
  for (int i = 1; i < n; i++)
    frame_unref(held[i]);
  as_destroy(as);
  CHECK_EQ(frames_free(), ALL_FREE);
}

static void test_unmap(void)
{
  machine_reset();
  struct addrspace *as = as_create();
  REQUIRE(as != NULL, "as_create returned NULL");
  size_t empty = frames_free();
  size_t f = data_frame('f'), g = data_frame('g');
  frame_ref(f);                                  /* keep f alive to look at it */
  REQUIRE(vm_map(as, 0x400000, f, PTE_W) == 0 && vm_map(as, 0x401000, g, PTE_W) == 0,
          "vm_map failed");
  uint8_t v;
  CHECK(load(as, 0x400000, &v) == 0 && v == 'f', "first page");     /* now in the TLB */
  CHECK(load(as, 0x401000, &v) == 0 && v == 'g', "second page");

  CHECK_EQ(vm_unmap(as, 0x400000), 0);
  CHECK_EQ(frame_refcount(f), 1);                /* the mapping's reference is gone */
  CHECK_EQ(load(as, 0x400000, &v), -1);          /* must fault, TLB or not */
  CHECK(load(as, 0x401000, &v) == 0 && v == 'g', "the other page must stay");
  CHECK_EQ(vm_unmap(as, 0x400000), -ENOENT);

  CHECK_EQ(vm_unmap(as, 0x401000), 0);           /* last page: all tables go */
  CHECK(frames_free() == empty - 1, "after unmapping everything %zu frame(s) are still "
        "used besides the PML4 (and the frame we hold)", empty - 1 - frames_free());
  CHECK_EQ(load(as, 0x401000, &v), -1);
  frame_unref(f);

  CHECK_EQ(vm_unmap(as, 0x9000000000), -ENOENT);  /* no tables there at all */
  CHECK_EQ(vm_unmap(as, 0x400010), -EINVAL);
  CHECK_NO_STALE_TLB();
  as_destroy(as);
  CHECK_EQ(frames_free(), ALL_FREE);
}

static void test_unmap_keeps_shared_tables(void)
{
  machine_reset();
  struct addrspace *as = as_create();
  REQUIRE(as != NULL, "as_create returned NULL");
  size_t start = frames_free();
  REQUIRE(vm_map(as, 0x400000, data_frame('a'), 0) == 0, "vm_map failed");     /* 3 tables */
  REQUIRE(vm_map(as, 0x40000000, data_frame('b'), 0) == 0, "vm_map failed");   /* +PD +PT */
  CHECK_EQ(start - frames_free(), 2 + 5);

  CHECK_EQ(vm_unmap(as, 0x400000), 0);           /* frees its PT and PD, not the PDPT */
  CHECK(start - frames_free() == 1 + 3, "expected 3 tables + 1 page left, have %zu frames",
        start - frames_free());
  uint8_t v;
  CHECK(load(as, 0x40000000, &v) == 0 && v == 'b', "the other mapping is gone");
  as_destroy(as);
  CHECK_EQ(frames_free(), ALL_FREE);
}

static void test_destroy_frees_everything(void)
{
  machine_reset();
  struct addrspace *as = as_create();
  REQUIRE(as != NULL, "as_create returned NULL");
  /* 24 pages spread over all four levels: 3 PDPTs, 6 PDs, 12 PTs */
  static const uint64_t pml4i[] = {0, 1, 255}, pdpti[] = {0, 5}, pdi[] = {0, 511}, pti[] = {1, 3};
  int n = 0;
  for (int a = 0; a < 3; a++)
    for (int b = 0; b < 2; b++)
      for (int c = 0; c < 2; c++)
        for (int d = 0; d < 2; d++) {
          uint64_t va = pml4i[a] << 39 | pdpti[b] << 30 | pdi[c] << 21 | pti[d] << 12;
          CHECK(vm_map(as, va, data_frame((unsigned char)n), n % 2 ? PTE_W : 0) == 0,
                "vm_map(%#llx) failed", (unsigned long long)va);
          n++;
        }
  CHECK_EQ(ALL_FREE - frames_free(), 1 + 21 + 24);
  as_destroy(as);
  CHECK(frames_free() == ALL_FREE, "as_destroy leaked %zu frame(s)", ALL_FREE - frames_free());
}

int main(void)
{
  RUN_TEST(test_create_destroy);
  RUN_TEST(test_map_and_access);
  RUN_TEST(test_intermediate_entries);
  RUN_TEST(test_protection);
  RUN_TEST(test_map_errors);
  RUN_TEST(test_map_out_of_memory);
  RUN_TEST(test_unmap);
  RUN_TEST(test_unmap_keeps_shared_tables);
  RUN_TEST(test_destroy_frees_everything);
  return test_summary();
}
