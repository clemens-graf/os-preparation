# A0-03 · heaplimit – a heap mapped on demand

**Time box 30 min · ★★☆ · read first: [ex03](../../examples/ex03-stack-growth/)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §3 sbrk & demand paging, §4 page faults

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         heaplimit
Nr:           1520 (sc_heaplimit)
Description:  Every process has a heap at HEAP_START = 0x600000000000. heaplimit(size)
              makes [HEAP_START, HEAP_START + size) usable (size rounded up to whole pages).
              Nothing is mapped by the syscall itself: a page of the heap is mapped (zeroed)
              by the page fault handler on its first access.
              An access to the heap region at or above the limit kills the process.
              Shrinking the limit unmaps all mapped heap pages above the new limit
              (growing it again gives fresh zero pages).
Parameters:   size_t size
Return Value: 0, or -1 if size > 1024 pages
Notes:        Put HEAP_START into nonstd.h so that tests can use it.
              The heap limit is per process and starts at 0.
```

## Test

`task start a0-03`, implement, `task test`. Done when 5 checks pass and the last access above the limit kills the process.

## Level 2 – hints

<details><summary>1 – where does a heap access end up today?</summary>

`PageFaultHandler::handlePageFault` → the address is "valid" (user, below `USER_BREAK`,
not present) → `Loader::loadPage` → no ELF segment covers it → `Syscall::exit(666)`. You
need a new branch *before* `loadPage` for addresses in the heap region.
</details>

<details><summary>2 – where do I store the limit?</summary>

It is per process → a member of your `UserProcess` (`currentThread->getProcess()`), initialised
to 0 in its constructor. Every thread of the process reaches the same object.
</details>

<details><summary>3 – locking: check-then-act</summary>

The page fault handler checks "address below the limit?" and then maps. If the check and
the map are in two separate critical sections, `heaplimit()` can shrink the heap in
between → a page stays mapped above the limit. Check and map under the same lock (the one
that also protects the page tables). And: never call `Syscall::exit` while holding a lock.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Add the constants (`HEAP_START`, the maximum size) and a per-process `heap_size_` (start 0) to `UserProcess.h`.
2. Add two methods to `UserProcess`: one that sets the limit, one that maps a heap page on demand.
3. Syscall plumbing for `heaplimit(size)`: it rounds up to whole pages and calls the set-limit method.
4. Set-limit: under `thread_lock_`: if the heap shrinks, unmap every heap page above the new limit and flush the TLB; store the new size.
5. Map-on-demand: allocate a frame **before** the lock; under `thread_lock_`: check "below the limit?" and `mapPage` **in the same critical section**; after the lock free the frame if it was not used.
6. Page fault handler: in the valid branch, **first** (before the stack check takes the lock): an address inside the heap region → call map-on-demand; outside the limit → kill the process (after releasing every lock); then `return`.
7. `task test` – the test touches pages below and above the limit and shrinks it.

</details>

## Your team's code

The task starts from your team's `main` (`osw26e3`, the commit in `team/BASE`) – the code you have in the interview.

- The heap limit is **per-process state → `UserProcess`** (`currentThread->getProcess()`):
  `heap_size_`, `mapHeapPage(vpn)` (check the limit and map, for the page fault handler) and
  `setHeapLimit(size)` (unmap everything above the new limit). Inside `UserProcess`:
  `arch_memory_->…` and `thread_lock_`.
- Your page fault handler (`PageFaultHandler.cpp`) is longer now: in the valid branch it takes
  `thread_lock_`, grows the stack (`isUserStackFault`), checks guard pages, and only then calls
  `getLoader().loadPage(address, getAddressSpace())`. Put the heap case **first** in the valid
  branch (before it takes the lock – `mapHeapPage` takes it itself) and `return` afterwards.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Walk me through what happens on the first write to <code>HEAP_START + 5000</code>.</summary>

CPU: #PF (not present, user, write) → `arch_pageFaultHandler` (asm, saves registers, `cr2` = address) → `pageFaultHandler` → `PageFaultHandler::enterPageFault` → `handlePageFault` → valid → **heap branch first**: `mapHeapPage(vpn)`: `allocPPN` (zeroed), under `thread_lock_`: below the limit? → `mapPage` → return. Back in user mode the same write instruction runs again and succeeds.

</details>

<details><summary>Two threads touch the same new heap page at the same moment – what happens?</summary>

Both fault, both allocate a frame, both take `thread_lock_` one after the other. The first maps; for the second `mapPage` returns `false` – it frees its frame and returns "inside" too. Both threads continue, the page is mapped once.

</details>

<details><summary>Why not just map all pages in the syscall? (memory, speed – and what Linux does: overcommit)</summary>

Lazy is cheaper: a program that sets a 100 MiB limit but touches 1 MiB uses 1 MiB of RAM, and the syscall is fast. Linux does the same – memory is only *promised* (overcommit) and mapped on first touch; the price is that memory can run out later, at a page fault.

</details>

<details><summary>What would you have to change so that the heap grows automatically on access (like a stack)?</summary>

Drop the limit check (or make the limit move): in the page fault handler map any fault inside the heap region – exactly what your team's stack growth does for the stack slot (`isUserStackFault`). A1-12 `sbrk` is the version with a moving limit.

</details>
