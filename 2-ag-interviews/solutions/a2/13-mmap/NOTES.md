# A2-13 anonymous mmap / munmap – solution notes

Patch: [`solution.patch`](solution.patch). Type S + E.

| Piece | Where |
|---|---|
| `MMAP_START 0x700000000000`, `MmapRegion mmap_regions_[16]`, `mmap_next_vpn_` | `UserProcess.h` |
| `mmapAnonymous(pages, writeable)`, `munmap(address, pages)`, `mapMmapPage(vpn)` | `UserProcess.cpp` (all under `thread_lock_`) |
| mmap case first in the valid branch | `PageFaultHandler.cpp` |
| `Syscall::mmap` (checks: length, `MAP_ANONYMOUS`, fd == -1), `case sc_munmap` | `Syscall.cpp` (1800/1801) |
| `mmap` → `__syscall(sc_mmap, length, prot, flags, fd)`, `munmap` | `userspace/libc/src/mman.c` |

```cpp
bool UserProcess::mapMmapPage(size_t vpn)
{
  size_t ppn = PageManager::instance()->allocPPN();        // before the lock
  thread_lock_.acquire();
  bool inside = false, mapped = false;
  for (MmapRegion& region : mmap_regions_)
    if (region.start_vpn && vpn >= region.start_vpn && vpn < region.start_vpn + region.pages)
    {
      inside = true;
      mapped = arch_memory_->mapPage(vpn, ppn, 1);           // check + map: one critical section
      if (mapped && !region.writeable) { /* clear writeable - still under the lock */ }
      break;
    }
  thread_lock_.release();
  if (!mapped) PageManager::instance()->freePPN(ppn);       // outside, or another thread was faster
  return inside;
}
```

- The same three pieces as the heap (A0-03) and `sbrk` (A1-12): state in `UserProcess`, a syscall
  that only changes the state, the page fault handler that maps on demand.
- `munmap`: `unmapPage` frees the frames itself; `flushTlb()` because rights were removed.
- The addresses are never reused (`mmap_next_vpn_` only grows, with a one-page gap): a stale
  pointer into an old mapping faults instead of silently hitting a new one.
- Fault after `munmap`: the vpn is in no mapping → not handled here → `loadPage` → no segment → 666.
- `fd` arrives as `(size_t) (ssize_t) -1` – compare with `(size_t) -1`.
