/*
 * vm.h - Module 08 assignment, parts A and B: the virtual-memory manager
 * of a small kernel, on the simulated machine of machine.h.
 *
 * Page-table entries use the real x86-64 layout (SWEB:
 * arch/x86/64/include/paging-definitions.h, there as bit fields):
 *
 *   63        52 51                          12 11  9 8     3 2 1 0
 *  +------------+------------------------------+-----+-------+-+-+-+
 *  |  (unused)  |  physical frame number (ppn) | avl |       |U|W|P|
 *  +------------+------------------------------+-----+-------+-+-+-+
 *
 * A virtual address picks one entry on each level:
 *
 *   47      39 38      30 29      21 20      12 11           0
 *  +----------+----------+----------+----------+--------------+
 *  | PML4 idx | PDPT idx |  PD idx  |  PT idx  | page offset  |
 *  +----------+----------+----------+----------+--------------+
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "machine.h"

#define PTE_P    0x001ULL                 /* present */
#define PTE_W    0x002ULL                 /* writable */
#define PTE_U    0x004ULL                 /* user mode may access */
#define PTE_COW  0x200ULL                 /* bit 9: ignored by the MMU, ours: copy-on-write */
#define PTE_ADDR 0x000FFFFFFFFFF000ULL    /* ppn << 12 */

#define PTE_PPN(pte)         (((pte) & PTE_ADDR) >> 12)
#define PTE_MAKE(ppn, flags) (((uint64_t)(ppn) << 12) | (flags))
/* level 3 = PML4, 2 = PDPT, 1 = PD, 0 = PT */
#define PTE_INDEX(vaddr, level) (((uint64_t)(vaddr) >> (12 + 9 * (level))) & 511)

/* Page-fault error code, bit for bit as on x86 (SWEB: PAGE_FAULT_*). */
#define PF_PRESENT 1   /* the page WAS present: a protection violation */
#define PF_WRITE   2   /* the access was a store */
#define PF_USER    4   /* from user mode (always, here) */

#define MAX_REGIONS 8

/* A range of user memory the process may use: [start, end). Pages in it
 * are allocated lazily, on the first access (zero-filled). */
struct region {
  uint64_t start, end;
  int writable;
};

struct addrspace {
  size_t root;                            /* ppn of the PML4 - what CR3 holds */
  struct region regions[MAX_REGIONS];
  int nregions;
};

/* ------------------------------ Part A ------------------------------ */

/* A new, empty address space (a zeroed PML4), or NULL if out of memory. */
struct addrspace *as_create(void);

/*
 * Pointer to the level-0 entry (in the PT) for vaddr, or NULL if a table
 * on the way is missing. With create != 0, missing tables are allocated,
 * zeroed and linked with P|W|U (the leaf entry decides the real rights);
 * NULL then means out of memory.
 */
uint64_t *vm_walk(struct addrspace *as, uint64_t vaddr, int create);

/*
 * Map the page at vaddr to frame ppn: present, user, plus `flags`
 * (a combination of PTE_W and PTE_COW). On success the mapping takes over
 * the caller's reference to ppn; on failure the caller still owns it.
 *   0; -EINVAL if vaddr is not page-aligned or not below USER_BREAK;
 *   -EEXIST if the page is already mapped; -ENOMEM if a page table cannot
 *   be allocated - and then no newly allocated table may be left behind.
 */
int vm_map(struct addrspace *as, uint64_t vaddr, size_t ppn, uint64_t flags);

/*
 * Remove the mapping of the page at vaddr and drop its frame reference.
 * Page tables that become completely empty are freed, bottom-up, as SWEB's
 * ArchMemory::unmapPage does (never the PML4). The TLB must not keep the
 * old translation. 0; -EINVAL for a bad vaddr (as in vm_map); -ENOENT if
 * the page was not mapped.
 */
int vm_unmap(struct addrspace *as, uint64_t vaddr);

/* Unmap everything, free all page tables, the PML4 and the struct. */
void as_destroy(struct addrspace *as);

/* ------------------------------ Part B ------------------------------ */

/*
 * Declare [start, start + len) usable, writable or read-only.
 *   0; -EINVAL if start or len is not page-aligned, len is 0, the range
 *   includes page 0 (NULL must always fault) or ends above USER_BREAK, or it
 *   overlaps an existing region; -ENOMEM if there are MAX_REGIONS already.
 */
int as_add_region(struct addrspace *as, uint64_t start, uint64_t len, int writable);

/*
 * Called by the CPU. Return 0 after fixing the cause (the CPU retries),
 * -1 for a segmentation fault. Cases:
 *   - not present, inside a region: allocate a frame, ZERO it, map it
 *     (writable iff the region is) - lazy allocation;
 *   - store to a present PTE_COW page: copy-on-write. If the frame is
 *     still shared, copy it into a new frame and map that writable;
 *     if this address space is its last user, just make it writable.
 *   - anything else (outside all regions, store to a read-only region,
 *     out of memory): -1.
 */
int page_fault(struct addrspace *as, uint64_t vaddr, int error);

/*
 * fork(): a copy of parent (regions and memory) that shares all frames.
 * Writable pages become read-only + PTE_COW in BOTH address spaces;
 * read-only pages are simply shared. No data frame is copied here.
 * Returns NULL if out of memory (leaving nothing allocated).
 */
struct addrspace *as_fork(struct addrspace *parent);
