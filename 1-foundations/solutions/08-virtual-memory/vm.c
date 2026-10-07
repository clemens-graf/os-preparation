/*
 * vm.c - Module 08, parts A and B: reference solution.
 */
#include "vm.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define USER_PML4_ENTRIES 256     /* the lower half of the PML4 is user space */

static uint64_t *table(size_t ppn)
{
  return phys(ppn);
}

/* A frame for a page table or a fresh page: never trust old contents. */
static size_t alloc_zeroed(void)
{
  size_t ppn = frame_alloc();
  if (ppn)
    memset(phys(ppn), 0, PAGE_SIZE);
  return ppn;
}

static int bad_user_page(uint64_t vaddr)
{
  return vaddr % PAGE_SIZE != 0 || vaddr >= USER_BREAK;
}

/* ------------------------------ Part A ------------------------------ */

struct addrspace *as_create(void)
{
  struct addrspace *as = calloc(1, sizeof *as);
  if (!as)
    return NULL;
  as->root = alloc_zeroed();
  if (!as->root) {
    free(as);
    return NULL;
  }
  return as;
}

uint64_t *vm_walk(struct addrspace *as, uint64_t vaddr, int create)
{
  size_t t = as->root;
  for (int level = 3; level >= 1; level--) {
    uint64_t *e = &table(t)[PTE_INDEX(vaddr, level)];
    if (!(*e & PTE_P)) {
      if (!create)
        return NULL;
      size_t next = alloc_zeroed();
      if (!next)
        return NULL;
      /* Generous rights on the upper levels: the MMU ANDs all levels,
       * so the leaf entry alone decides (SWEB's mapPage does the same). */
      *e = PTE_MAKE(next, PTE_P | PTE_W | PTE_U);
    }
    t = PTE_PPN(*e);
  }
  return &table(t)[PTE_INDEX(vaddr, 0)];
}

/* How many page tables would vm_walk(create) have to allocate? */
static int tables_missing(struct addrspace *as, uint64_t vaddr)
{
  size_t t = as->root;
  for (int level = 3; level >= 1; level--) {
    uint64_t e = table(t)[PTE_INDEX(vaddr, level)];
    if (!(e & PTE_P))
      return level;             /* this table's child and everything below */
    t = PTE_PPN(e);
  }
  return 0;
}

int vm_map(struct addrspace *as, uint64_t vaddr, size_t ppn, uint64_t flags)
{
  if (bad_user_page(vaddr))
    return -EINVAL;
  /* Check first, allocate afterwards: then there is nothing to roll back. */
  if (frames_free() < (size_t)tables_missing(as, vaddr))
    return -ENOMEM;
  uint64_t *pte = vm_walk(as, vaddr, 1);
  if (*pte & PTE_P)
    return -EEXIST;
  /* No TLB work needed: the TLB never caches "not present". */
  *pte = PTE_MAKE(ppn, PTE_P | PTE_U | (flags & (PTE_W | PTE_COW)));
  return 0;
}

static int table_empty(size_t t)
{
  for (int i = 0; i < 512; i++)
    if (table(t)[i] & PTE_P)
      return 0;
  return 1;
}

int vm_unmap(struct addrspace *as, uint64_t vaddr)
{
  if (bad_user_page(vaddr))
    return -EINVAL;

  size_t tables[4];             /* tables[level]: the table used on that level */
  tables[3] = as->root;
  for (int level = 3; level >= 1; level--) {
    uint64_t e = table(tables[level])[PTE_INDEX(vaddr, level)];
    if (!(e & PTE_P))
      return -ENOENT;
    tables[level - 1] = PTE_PPN(e);
  }
  uint64_t *pte = &table(tables[0])[PTE_INDEX(vaddr, 0)];
  if (!(*pte & PTE_P))
    return -ENOENT;

  size_t ppn = PTE_PPN(*pte);
  *pte = 0;
  tlb_invalidate(vaddr);        /* before the frame can be reused */
  frame_unref(ppn);

  /* Free empty tables bottom-up: PT, then PD, then PDPT - never the PML4. */
  for (int level = 0; level <= 2 && table_empty(tables[level]); level++) {
    table(tables[level + 1])[PTE_INDEX(vaddr, level + 1)] = 0;
    frame_unref(tables[level]);
  }
  return 0;
}

static void free_tree(size_t t, int level)
{
  int n = level == 3 ? USER_PML4_ENTRIES : 512;
  for (int i = 0; i < n; i++) {
    uint64_t e = table(t)[i];
    if (!(e & PTE_P))
      continue;
    if (level > 0)
      free_tree(PTE_PPN(e), level - 1);
    else
      frame_unref(PTE_PPN(e));  /* shared (COW) frames survive: refcount */
  }
  frame_unref(t);
}

void as_destroy(struct addrspace *as)
{
  if (!as)
    return;
  /* Freeing the PML4 also drops this address space's TLB entries (the
   * machine's stand-in for "the kernel no longer runs on this CR3"). */
  free_tree(as->root, 3);
  free(as);
}

/* ------------------------------ Part B ------------------------------ */

int as_add_region(struct addrspace *as, uint64_t start, uint64_t len, int writable)
{
  if (start % PAGE_SIZE || len % PAGE_SIZE || len == 0 || start < PAGE_SIZE)
    return -EINVAL;
  if (len > USER_BREAK || start > USER_BREAK - len)   /* no start + len overflow */
    return -EINVAL;
  for (int i = 0; i < as->nregions; i++)
    if (start < as->regions[i].end && as->regions[i].start < start + len)
      return -EINVAL;
  if (as->nregions == MAX_REGIONS)
    return -ENOMEM;
  as->regions[as->nregions++] = (struct region){start, start + len, writable};
  return 0;
}

static struct region *find_region(struct addrspace *as, uint64_t vaddr)
{
  for (int i = 0; i < as->nregions; i++)
    if (vaddr >= as->regions[i].start && vaddr < as->regions[i].end)
      return &as->regions[i];
  return NULL;
}

int page_fault(struct addrspace *as, uint64_t vaddr, int error)
{
  struct region *r = find_region(as, vaddr);
  if (!r)
    return -1;                                  /* segmentation fault */
  uint64_t page = vaddr & ~(PAGE_SIZE - 1);

  if (!(error & PF_PRESENT)) {
    /* Lazy allocation. Zeroing is not optional: the frame may hold
     * another process's data. */
    size_t ppn = alloc_zeroed();
    if (!ppn)
      return -1;
    if (vm_map(as, page, ppn, r->writable ? PTE_W : 0) != 0) {
      frame_unref(ppn);
      return -1;
    }
    return 0;
  }

  uint64_t *pte = vm_walk(as, page, 0);
  if (!(error & PF_WRITE) || !pte || !(*pte & PTE_COW))
    return -1;                                  /* store to a read-only region */

  size_t old = PTE_PPN(*pte);
  if (frame_refcount(old) == 1) {
    /* Everybody else has copied already: the page is ours alone. */
    *pte = (*pte & ~PTE_COW) | PTE_W;
    tlb_invalidate(page);
    return 0;
  }
  size_t copy = frame_alloc();
  if (!copy)
    return -1;
  memcpy(phys(copy), phys(old), PAGE_SIZE);
  *pte = PTE_MAKE(copy, PTE_P | PTE_U | PTE_W);
  tlb_invalidate(page);
  frame_unref(old);
  return 0;
}

/* Share every mapped page below `base` of this subtree with the child. */
static int fork_tree(struct addrspace *child, size_t t, int level, uint64_t base)
{
  int n = level == 3 ? USER_PML4_ENTRIES : 512;
  for (int i = 0; i < n; i++) {
    uint64_t *e = &table(t)[i];
    if (!(*e & PTE_P))
      continue;
    uint64_t vaddr = base | ((uint64_t)i << (12 + 9 * level));
    if (level > 0) {
      if (fork_tree(child, PTE_PPN(*e), level - 1, vaddr) != 0)
        return -1;
      continue;
    }
    if (*e & PTE_W) {
      *e = (*e & ~PTE_W) | PTE_COW;
      tlb_invalidate(vaddr);    /* the parent's TLB may still say "writable" */
    }
    size_t ppn = PTE_PPN(*e);
    frame_ref(ppn);
    if (vm_map(child, vaddr, ppn, *e & PTE_COW) != 0) {
      frame_unref(ppn);
      return -1;
    }
  }
  return 0;
}

struct addrspace *as_fork(struct addrspace *parent)
{
  struct addrspace *child = as_create();
  if (!child)
    return NULL;
  memcpy(child->regions, parent->regions, sizeof parent->regions);
  child->nregions = parent->nregions;
  if (fork_tree(child, parent->root, 3, 0) != 0) {
    /* Pages of the parent that became COW stay so; harmless: with
     * refcount 1 the next store just makes them writable again. */
    as_destroy(child);
    return NULL;
  }
  return child;
}
