# Module 08 — Virtual memory

**Time:** 4–5 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 8)

Every thread you create with `pthread_create` lives in its parent's
address space and needs its own stack in it; A2 of the course is largely
about memory (page faults, copy-on-write `fork`, swapping). This module
builds the machinery on a simulated x86-64 MMU that behaves like SWEB's
hardware: same entry format, same four levels, a TLB that you must keep
in sync.

## Steps

1. **Read** chapter 8 (address translation, the x86-64 page-table format,
   TLB and invalidation, page faults, demand paging, copy-on-write,
   page replacement: FIFO, LRU, OPT, Clock, Belady's anomaly; thrashing).
2. **Examples** — `cd example && make run`:
   - `demand_paging.c` — 1 GiB promised, frames delivered page by page
   - `cow_fork.c` — fork copies nothing; the first write per page does
   - `user_fault.c` — a page-fault handler in user space (SIGSEGV + mprotect)
3. **Assignment** — `assignment/`:
   - Part A `vm.c`: page walk, `vm_map`, `vm_unmap` (freeing empty
     tables like SWEB's `unmapPage`), `as_destroy`
   - Part B `vm.c`: regions, the page-fault handler (lazy zero-filled
     pages, segmentation faults), `fork` with copy-on-write
   - Part C `replace.c`: FIFO, LRU, OPT and Clock on reference strings
   ```bash
   cd assignment && make test     # or test-pagetable / test-fault / test-replace
   ```
   The simulated machine aborts with an explanation when a page table
   contains junk, and reports every access through a stale TLB entry.
4. **Paper questions** — `assignment/questions.md` (Q9 and Q10 are
   about your P1 work in SWEB).

## Done when

- `make test` prints `==== all parts pass`.
- You can compute page-table indices by hand and explain why `fork` must
  invalidate the *parent's* TLB entries.

Reference solution and answers: `../solutions/08-virtual-memory/`.
