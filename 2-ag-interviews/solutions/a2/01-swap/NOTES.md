# A2-01 swap a page out and back in – solution notes

Patch: [`solution.patch`](solution.patch). This is the base of A2-02 … A2-06.

## Pieces

| Piece | Where |
|---|---|
| `PTE_SWAPPED` marker, `forEachSwappedPage`, "table in use" = any non-zero entry | `ArchMemory.h/.cpp` |
| `SwapManager`: `idea2`, slot bitmap, `writeOut(ppn)`, `readIn(slot, ppn)`, `freeSlot`, created in `startup()` after the block devices | new `common/{include,source}/mm/SwapManager.*` |
| `Loader::swapOut(vpn)`, `Loader::swapIn(vpn)`, slot cleanup in `~Loader` | `Loader.h/.cpp` |
| page fault: `if (!swapIn(vpn)) loadPage(address)` | `PageFaultHandler.cpp` |
| `swapout`, `swapinfo` | syscalls + `nonstd.*` |

## Swapped-out PTE

```
present = 0          -> the CPU ignores everything else in the entry
ignored_2 = 1        -> "swapped" (PTE_SWAPPED)
page_ppn = slot      -> where on the device
writeable, user_access unchanged -> correct again after swap-in
```

## swapOut

```cpp
ScopeLock lock(arch_memory_lock_);
/* resolve, must be present */
pte.present = 0;                         // 1. nobody can use it any more ...
ArchMemory::flushTlb();                  //    ... not even through the TLB
size_t slot = SwapManager::instance()->writeOut(ppn);   // 2. disk I/O, page-table lock held
if (slot == -1) { pte.present = 1; return -1; }
pte.page_ppn = slot; pte.ignored_2 = PTE_SWAPPED;      // 3. remember where
PageManager::instance()->freePPN(ppn);
```

## swapIn (called first in the valid page fault branch)

Under the lock: present already → `true` (another thread was faster). Not swapped → `false`
(normal fault → `loadPage`). Swapped → allocPPN, `readIn`, `freeSlot`, `page_ppn = ppn`,
marker off, `present = 1` **last**.

## Locking

- Lock order everywhere: `arch_memory_lock_` → `PageManager` (SpinLock, inside allocPPN/freePPN)
  → `SwapManager::lock_`. Never the other way round.
- Disk I/O while holding the page-table lock (a Mutex): allowed – the request yields until the IDE
  interrupt. It blocks the process' other page faults meanwhile – exactly what keeps them off the
  half-written page. A SpinLock here would be wrong (spinning through a whole disk request).
- `SwapManager::lock_` serialises the device requests and protects the bitmap.

## The trap you only find by reading `unmapPage`

`checkAndRemove` freed a page table when no entry was *present*. Swapped entries are not present →
the table (and with it the slot number) would be freed when another page in it is unmapped. Fixed
by treating every non-zero entry as "in use".

## Pitfalls

- Writing to disk while the page is still present (torn copy).
- Forgetting the slots at exit (`swapinfo` never returns to 0).
- Calling `loadPage` for a swapped page: it maps a fresh copy from the binary over the swap marker.
- Kernel-mode faults: a syscall writing into a swapped-out user buffer goes through the same path –
  works, because the page fault handler runs with interrupts on and may do disk I/O.
