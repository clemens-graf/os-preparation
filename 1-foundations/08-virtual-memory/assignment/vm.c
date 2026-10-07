/*
 * vm.c - Module 08 assignment, parts A and B. Replace the TODOs.
 * Test:  make test-pagetable  (part A)   make test-fault  (part B)
 *
 * Page tables live in frames: phys(ppn) gives you a pointer to one, i.e.
 * an array of 512 uint64_t entries. SWEB's ArchMemory.cpp does the same
 * with getIdentAddressOfPPN() - read its mapPage() and unmapPage().
 */
#include "vm.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------ Part A ------------------------------ */

struct addrspace *as_create(void)
{
  /* TODO: allocate the struct and a PML4 frame. Remember: frame_alloc
   * returns a frame full of whatever was there before. */
  return NULL;
}

uint64_t *vm_walk(struct addrspace *as, uint64_t vaddr, int create)
{
  /* TODO: from as->root down through levels 3, 2, 1 using PTE_INDEX;
   * at each level, a missing entry means NULL - or, with create, a new
   * zeroed table linked in with P|W|U. Return a pointer into the PT. */
  return NULL;
}

int vm_map(struct addrspace *as, uint64_t vaddr, size_t ppn, uint64_t flags)
{
  /* TODO: check vaddr; walk with create; -EEXIST if present; write the
   * entry. -ENOMEM: a partial walk may already have created tables -
   * free the new ones again (or check frames_free() first). */
  return -ENOSYS;
}

int vm_unmap(struct addrspace *as, uint64_t vaddr)
{
  /* TODO: clear the entry, frame_unref the page, tlb_invalidate, then
   * free every table that became empty, bottom-up (not the PML4).
   * You need the tables of all four levels: remember them on the way
   * down instead of calling vm_walk. */
  return -ENOSYS;
}

void as_destroy(struct addrspace *as)
{
  /* TODO: walk the whole tree (only the lower half of the PML4 holds user
   * space - SWEB's ~ArchMemory loops over it the same way), unref every
   * mapped frame and every table, then the PML4 and the struct. */
}

/* ------------------------------ Part B ------------------------------ */

int as_add_region(struct addrspace *as, uint64_t start, uint64_t len, int writable)
{
  /* TODO */
  return -ENOSYS;
}

int page_fault(struct addrspace *as, uint64_t vaddr, int error)
{
  /* TODO: see vm.h. Every PTE you change here must also leave the TLB. */
  return -1;
}

struct addrspace *as_fork(struct addrspace *parent)
{
  /* TODO: new address space, same regions; for every mapped user page:
   * share the frame (frame_ref), writable ones become read-only + COW in
   * the parent AND in the child. The parent's TLB may still say
   * "writable" for those pages. */
  return NULL;
}
