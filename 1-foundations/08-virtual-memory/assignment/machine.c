/*
 * machine.c - the simulated hardware for parts A and B. Given; do not edit.
 * Not thread-safe: the tests use one thread.
 */
#include "machine.h"
#include "vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t machine_page_faults;
size_t machine_tlb_stale;

static void __attribute__((format(printf, 1, 2), noreturn)) die(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "machine: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  abort();
}

/* ---------------------------- physical memory ---------------------------- */

static unsigned char ram[NFRAMES][PAGE_SIZE] __attribute__((aligned(4096)));
static unsigned refcount[NFRAMES];
static size_t free_stack[NFRAMES];        /* LIFO: the last freed frame is reused first */
static size_t nfree;

size_t frame_alloc(void)
{
  if (nfree == 0)
    return 0;
  size_t ppn = free_stack[--nfree];
  refcount[ppn] = 1;
  return ppn;
}

void frame_ref(size_t ppn)
{
  if (ppn == 0 || ppn >= NFRAMES || refcount[ppn] == 0)
    die("frame_ref(%zu): not an allocated frame", ppn);
  refcount[ppn]++;
}

static void tlb_forget_root(size_t root);

void frame_unref(size_t ppn)
{
  if (ppn == 0 || ppn >= NFRAMES || refcount[ppn] == 0)
    die("frame_unref(%zu): not an allocated frame (freed twice?)", ppn);
  if (--refcount[ppn] == 0) {
    free_stack[nfree++] = ppn;
    tlb_forget_root(ppn);     /* the kernel would have left that CR3 by now */
  }
}

unsigned frame_refcount(size_t ppn)
{
  return ppn < NFRAMES ? refcount[ppn] : 0;
}

size_t frames_free(void)
{
  return nfree;
}

void *phys(size_t ppn)
{
  if (ppn == 0 || ppn >= NFRAMES)
    die("phys(%#zx): no such frame - a frame number read from a page-table entry "
        "full of junk? (frame_alloc does not zero tables for you)", ppn);
  return ram[ppn];
}

/* ---------------------------------- TLB ---------------------------------- */

#define TLB_SIZE 16

static struct tlb_entry {
  int valid;
  size_t root;
  uint64_t vpn;
  size_t ppn;
  int writable;
} tlb[TLB_SIZE];
static int tlb_next;

void tlb_invalidate(uint64_t vaddr)
{
  for (int i = 0; i < TLB_SIZE; i++)
    if (tlb[i].valid && tlb[i].vpn == vaddr / PAGE_SIZE)
      tlb[i].valid = 0;
}

void tlb_flush(void)
{
  memset(tlb, 0, sizeof tlb);
}

static void tlb_forget_root(size_t root)
{
  for (int i = 0; i < TLB_SIZE; i++)
    if (tlb[i].root == root)
      tlb[i].valid = 0;
}

/* ---------------------------------- MMU ---------------------------------- */

/* The hardware page walk. Returns 0 and the frame, or a page-fault error code. */
static int walk(size_t root, uint64_t vaddr, int write, size_t *ppn, int *writable)
{
  int err_rw = write ? PF_WRITE : 0;
  if (vaddr >= USER_BREAK)
    return PF_USER | err_rw;                  /* kernel half: never mapped for the user */
  size_t table = root;
  int w = 1;
  for (int level = 3; level >= 0; level--) {
    if (refcount[table] == 0)
      die("page walk for %#lx: page table in frame %zu, which is FREE",
          (unsigned long)vaddr, table);
    uint64_t pte = ((uint64_t *)ram[table])[PTE_INDEX(vaddr, level)];
    if (!(pte & PTE_P))
      return PF_USER | err_rw;
    if (!(pte & PTE_U))
      return PF_USER | err_rw | PF_PRESENT;
    w &= (pte & PTE_W) != 0;
    table = PTE_PPN(pte);
    if (table == 0 || table >= NFRAMES)
      die("page walk for %#lx: a level-%d entry points to frame %#zx - garbage in a page "
          "table that was never zeroed?", (unsigned long)vaddr, level, table);
  }
  if (refcount[table] == 0)
    die("page walk for %#lx: mapped to frame %zu, which is FREE", (unsigned long)vaddr, table);
  if (write && !w)
    return PF_USER | PF_WRITE | PF_PRESENT;
  *ppn = table;
  *writable = w;
  return 0;
}

/* Translate with the TLB; on a TLB hit, compare with the tables to catch
 * stale entries (real hardware would silently use them - and so do we). */
static int translate(struct addrspace *as, uint64_t vaddr, int write, size_t *ppn)
{
  uint64_t vpn = vaddr / PAGE_SIZE;
  for (int i = 0; i < TLB_SIZE; i++) {
    struct tlb_entry *e = &tlb[i];
    if (!e->valid || e->root != as->root || e->vpn != vpn)
      continue;
    if (write && !e->writable) {             /* not enough rights: re-walk */
      e->valid = 0;
      break;
    }
    size_t real_ppn = 0;
    int real_w = 0;
    int err = walk(as->root, vaddr, 0, &real_ppn, &real_w);
    if (err || real_ppn != e->ppn || (write && !real_w)) {
      machine_tlb_stale++;
      fprintf(stderr, "machine: %s at %#lx used a STALE TLB entry (frame %zu%s); the page "
              "tables say: %s. Missing tlb_invalidate() after changing a PTE?\n",
              write ? "store" : "load", (unsigned long)vaddr, e->ppn,
              e->writable ? ", writable" : "",
              err ? "not accessible" : real_ppn != e->ppn ? "another frame" : "read-only");
    }
    *ppn = e->ppn;
    return 0;
  }
  int w = 0;
  int err = walk(as->root, vaddr, write, ppn, &w);
  if (err)
    return err;
  tlb[tlb_next] = (struct tlb_entry){1, as->root, vpn, *ppn, w};
  tlb_next = (tlb_next + 1) % TLB_SIZE;
  return 0;
}

static unsigned char *access_byte(struct addrspace *as, uint64_t vaddr, int write)
{
  for (int attempt = 0; attempt < 4; attempt++) {
    size_t ppn;
    int err = translate(as, vaddr, write, &ppn);
    if (!err)
      return &ram[ppn][vaddr % PAGE_SIZE];
    machine_page_faults++;
    if (page_fault(as, vaddr, err) != 0)
      return NULL;
  }
  die("the %s at %#lx still faults after page_fault() returned 0 four times",
      write ? "store" : "load", (unsigned long)vaddr);
}

int cpu_load(struct addrspace *as, uint64_t vaddr, uint8_t *val)
{
  unsigned char *b = access_byte(as, vaddr, 0);
  if (!b)
    return -1;
  *val = *b;
  return 0;
}

int cpu_store(struct addrspace *as, uint64_t vaddr, uint8_t val)
{
  unsigned char *b = access_byte(as, vaddr, 1);
  if (!b)
    return -1;
  *b = val;
  return 0;
}

/* ------------------------------ test harness ------------------------------ */

void machine_reset(void)
{
  /* Junk everywhere: every "present" bit set, frame numbers far out of range. */
  memset(ram, 0xA5, sizeof ram);
  memset(refcount, 0, sizeof refcount);
  nfree = 0;
  for (size_t ppn = NFRAMES - 1; ppn >= 1; ppn--)
    free_stack[nfree++] = ppn;
  tlb_flush();
  tlb_next = 0;
  machine_page_faults = 0;
  machine_tlb_stale = 0;
}
