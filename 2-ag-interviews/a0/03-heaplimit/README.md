# A0-03 · heaplimit – a heap mapped on demand

**Time box 30 min · ★★☆ · read first: [ex03](../../examples/ex03-stack-growth/)**

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

## Hints

<details><summary>1 – where does a heap access end up today?</summary>

`PageFaultHandler::handlePageFault` → the address is "valid" (user, below `USER_BREAK`,
not present) → `Loader::loadPage` → no ELF segment covers it → `Syscall::exit(666)`. You
need a new branch *before* `loadPage` for addresses in the heap region.
</details>

<details><summary>2 – where do I store the limit?</summary>

It is per process. In base SWEB the per-process object reachable from every thread is the
`Loader` (`currentThread->loader_`) – the solutions put process state there. (In your
team repo it belongs into `UserProcess`.)
</details>

<details><summary>3 – locking: check-then-act</summary>

The page fault handler checks "address below the limit?" and then maps. If the check and
the map are in two separate critical sections, `heaplimit()` can shrink the heap in
between → a page stays mapped above the limit. Check and map under the same lock (the one
that also protects the page tables). And: never call `Syscall::exit` while holding a lock.
</details>

## Questions a tutor might ask

- Walk me through what happens on the first write to `HEAP_START + 5000`.
- Two threads touch the same new heap page at the same moment – what happens?
- Why not just map all pages in the syscall? (memory, speed – and what Linux does: overcommit)
- What would you have to change so that the heap grows automatically on access (like a stack)?
