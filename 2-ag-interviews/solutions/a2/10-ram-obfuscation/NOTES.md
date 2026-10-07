# A2-10 RAM obfuscation thread – solution notes

Patch: [`solution.patch`](solution.patch).

## Pieces

| Piece | Where |
|---|---|
| all loaders in an intrusive list: `static Loader* all_loaders_`, `static Mutex* all_loaders_lock_` (created in `startup()`), `next_loader_` | `Loader.h/.cpp` (ctor registers, dtor unregisters FIRST) |
| `Loader::relocatePage(index)`, `Loader::isMapped(vpn)` | `Loader.cpp` |
| `ObfuscationThread` (kernel thread, every 2 ticks: random loader, random present page) | new `ObfuscationThread.*`, started in `startup()` |
| page fault: `if (!isMapped(vpn)) loadPage(address)` | `PageFaultHandler.cpp` |

## relocatePage

```cpp
ScopeLock lock(arch_memory_lock_);
/* pick a present page */
pte.present = 0;                 // 1. a write to the old page during the copy would be lost
ArchMemory::flushTlb();
new_ppn = allocPPN(); memcpy(ident(new_ppn), ident(old_ppn), PAGE_SIZE);   // 2. copy
pte.page_ppn = new_ppn; pte.present = 1;                                    // 3. remap
freePPN(old_ppn);
```

## "Take special care with pagefaults during the remap"

The kernel thread can be preempted in the middle of the copy. A thread of that process runs and
touches the page → not present → page fault. Without care the handler calls `loadPage`, which maps a
fresh page from the binary (data loss) – or, for a stack page, finds no segment and **kills the
process**. With `isMapped(vpn)` first: it takes the page-table lock, i.e. waits until the relocation
is done, sees the page present, returns – the instruction is repeated and works.

## Locking and lifetime

- Lock order: `all_loaders_lock_` → one loader's `arch_memory_lock_`. The page fault path takes only
  the latter; the loader's destructor takes only the former. No cycle.
- The thread holds `all_loaders_lock_` while using a loader, and the destructor unregisters under that
  lock *first* – so a loader can never be destroyed while the thread works on it.
- TLB of the target process: on one CPU it is not running while the kernel thread runs, and its TLB
  entries are flushed when its `cr3` is loaded again. On several CPUs you would need a TLB shootdown
  (IPI) – a good interview answer.
