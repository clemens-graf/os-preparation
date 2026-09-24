/*
 * machine.h - the simulated hardware for parts A and B. Given; do not edit.
 *
 * Physical memory is NFRAMES frames of 4 KiB. The simulated CPU runs user
 * code in an address space whose page tables YOU build in those frames,
 * in exactly the x86-64 format (4 levels, 512 entries of 8 bytes each;
 * see vm.h). Like the real MMU it
 *   - walks PML4 -> PDPT -> PD -> PT on a TLB miss,
 *   - requires P and U on every level, and W on every level for a store,
 *   - caches translations in a small TLB and does NOT notice when you
 *     change a page-table entry later - you must invalidate,
 *   - raises a page fault by calling YOUR page_fault(), then retries.
 *
 * The simulator also checks what real hardware cannot: it complains
 * loudly about page tables that contain garbage, point to free frames,
 * and about accesses through stale TLB entries.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE  4096UL
#define NFRAMES    256            /* 1 MiB of "RAM"; frame 0 is never handed out */
#define USER_BREAK 0x0000800000000000UL

struct addrspace;                 /* vm.h */

/* ------------------ for the kernel (your code) ------------------ */

/*
 * The frame allocator (SWEB: PageManager::allocPPN / freePPN) with
 * reference counts. frame_alloc returns a frame with refcount 1, or 0 when
 * memory is full. Its CONTENTS ARE WHATEVER WAS THERE BEFORE - often an
 * old page of another process. frame_unref at refcount 1 frees the frame.
 */
size_t   frame_alloc(void);
void     frame_ref(size_t ppn);
void     frame_unref(size_t ppn);
unsigned frame_refcount(size_t ppn);
size_t   frames_free(void);

/* Where the kernel sees a frame's bytes (SWEB: ArchMemory::getIdentAddressOfPPN). */
void *phys(size_t ppn);

/* invlpg: drop cached translations of vaddr's page (in all address spaces). */
void tlb_invalidate(uint64_t vaddr);
/* Drop the whole TLB (like reloading CR3). */
void tlb_flush(void);

/* ------------------ the CPU (called by the tests) ------------------ */

/*
 * One user-mode load / store at vaddr in address space as (CR3 = as->root).
 * On a page fault the CPU calls page_fault(as, vaddr, error) and retries
 * the access. Returns 0 on success, -1 if page_fault gave up (the process
 * would get SIGSEGV).
 */
int cpu_load(struct addrspace *as, uint64_t vaddr, uint8_t *val);
int cpu_store(struct addrspace *as, uint64_t vaddr, uint8_t val);

/* ------------------ for the tests only ------------------ */

void machine_reset(void);          /* all frames free (and full of junk), TLB empty */
extern size_t machine_page_faults; /* page_fault() calls so far */
extern size_t machine_tlb_stale;   /* accesses through a TLB entry the tables no longer back */
