# A0-02 allocpage / freepage – solution notes

Patch: [`solution.patch`](solution.patch).

## Files touched

`syscall-definitions.h` (1510, 1511), `Syscall.h`, `Syscall.cpp` (two cases, `allocPage`,
`freePage`), `Loader.h/.cpp` (page-table lock), `nonstd.h/.c` (wrappers).

## The code

```cpp
size_t Syscall::allocPage()
{
  Loader* loader = currentThread->loader_;
  size_t first_vpn = ALLOC_REGION_START / PAGE_SIZE;
  size_t ppn = PageManager::instance()->allocPPN();      // outside the lock

  {
    ScopeLock lock(loader->arch_memory_lock_);
    for (size_t vpn = first_vpn; vpn < first_vpn + ALLOC_REGION_PAGES; ++vpn)
    {
      if (loader->arch_memory_.resolveMapping(vpn).page_size != 0)
        continue;
      bool mapped = loader->arch_memory_.mapPage(vpn, ppn, 1);
      assert(mapped && "we hold the lock and the slot was free");
      return vpn * PAGE_SIZE;
    }
  }
  PageManager::instance()->freePPN(ppn);                // region full: give it back
  return 0;
}

size_t Syscall::freePage(size_t address)
{
  if (address % PAGE_SIZE != 0 || address < ALLOC_REGION_START ||
      address >= ALLOC_REGION_START + ALLOC_REGION_PAGES * PAGE_SIZE)
    return (size_t) -1;
  Loader* loader = currentThread->loader_;
  ScopeLock lock(loader->arch_memory_lock_);
  if (loader->arch_memory_.resolveMapping(address / PAGE_SIZE).page_size == 0)
    return (size_t) -1;
  loader->arch_memory_.unmapPage(address / PAGE_SIZE);  // frees the physical page too
  ArchMemory::flushTlb();
  return 0;
}
```

- `allocPPN()` returns a **zeroed** page (`PageManager` memsets it) – no need to clear it.
- `mapPage(vpn, ppn, user_access)`: page **numbers**; `user_access = 1` or the user cannot
  touch it. It returns `false` if the vpn is already mapped – then the caller still owns `ppn`.
- `unmapPage(vpn)` frees the physical page and empty page-table pages. It does **not**
  flush the TLB → `ArchMemory::flushTlb()` (reloads `cr3`).
- `ScopeLock` releases the mutex at every `return` – no forgotten `release()` on an early return.

## Locking

**Search + map = one critical section.** Without it: thread A finds slot 7 free, releases
the lock, thread B finds slot 7 free too, A maps it, B's `mapPage` returns false – B
returns slot 7, which A owns. The assert documents the reasoning: we hold the lock and saw
the slot free, so `mapPage` cannot fail.

`allocPPN` is outside the lock: keeps the critical section short, and the lock order
"page-table lock → PageManager lock" still holds inside `mapPage` (it allocates page-table
pages) – we never take them the other way round.

## Pitfalls

- Freeing the physical page yourself after `unmapPage` → *Double free PPN* assertion.
- Passing an address to `mapPage`/`resolveMapping` instead of a page number →
  "This is not a valid vpn" assertion or a mapping far away.
- No TLB flush → the freed page may stay usable for a while (and is already given to
  somebody else!). The test's last access may then even *not* fault.
- Not checking alignment/region in `freepage` → the user could unmap its own code page.

## Answers to the tutor questions

- *Page-table pages?* `unmapPage` → `checkAndRemove` frees PT/PD/PDPT pages that became empty.
- *TLB?* The TLB caches translations; an unmapped entry can stay cached until `cr3` is
  reloaded or `invlpg`. A context switch to another process also flushes it (cr3 write),
  but a switch to another thread of the **same** process does not.
- *Leftover pages at exit?* `ArchMemory::~ArchMemory` frees every present page of the lower half.
