# A0-01 meminfo – solution notes

Patch: [`solution.patch`](solution.patch) (against upstream `f6fcb2ab`).

## Files touched

| File | What |
|---|---|
| `common/include/kernel/syscall-definitions.h` | `#define sc_meminfo 1500` – shared by kernel and libc |
| `common/include/kernel/Syscall.h` | `static size_t memInfo(size_t user_info);` |
| `common/source/kernel/Syscall.cpp` | `case sc_meminfo:` + `memInfo()` |
| `arch/x86/64/include/ArchMemory.h`, `.../source/ArchMemory.cpp` | `countUserPages()` – the page-table walk |
| `common/include/kernel/Loader.h`, `Loader.cpp` | `Mutex arch_memory_lock_` (the page-table lock, used in all A0 solutions) |
| `userspace/libc/include/nonstd.h` | `#include "types.h"`, `struct meminfo`, declaration |
| `userspace/libc/src/nonstd.c` | wrapper: `__syscall(sc_meminfo, (size_t) info, 0, 0, 0, 0)` |

## The code

```cpp
size_t Syscall::memInfo(size_t user_info)
{
  if (user_info == 0 || user_info >= USER_BREAK - sizeof(MemInfo))
    return (size_t) -1;

  MemInfo info;
  info.total_pages = PageManager::instance()->getTotalNumPages();
  info.free_pages = PageManager::instance()->getNumFreePages();
  {
    Loader* loader = currentThread->loader_;
    ScopeLock lock(loader->arch_memory_lock_);
    info.process_pages = loader->arch_memory_.countUserPages();
  }
  *(MemInfo*) user_info = info;
  return 0;
}
```

- **Pointer check without overflow:** `user_info + sizeof > USER_BREAK` could wrap around for
  huge values; `user_info >= USER_BREAK - sizeof` cannot.
- **Fill a kernel copy, then copy once.** The user write is the only thing that can page
  fault (if the page is not mapped yet, it is loaded – or the process is killed if no
  segment covers it). Doing it after the lock is released means a fault never happens
  while we hold a lock.
- `countUserPages()` is the loop of `ArchMemory::~ArchMemory()` – 4 levels, lower half of
  the PML4 only – counting `pt[pti].present` instead of freeing.
- `(size_t) -1`, **not** `-1U`: `-1U` is `0xFFFFFFFF`; `__syscall` returns 64 bit, so user space
  would see 4294967295, not -1. (The base code uses `-1U` in a few places – a classic.)

## Locking

| Data | Lock | Why |
|---|---|---|
| page tables of the process (`arch_memory_`) | `Loader::arch_memory_lock_` (Mutex) | another thread of the process (A1) or a page fault could map/unmap during the walk; `unmapPage` even frees page-table pages – we would read freed memory |
| `PageManager` counters | `PageManager`'s own SpinLock inside the getters | the two numbers are not read atomically together – fine for statistics |

The lock is a **Mutex**, not a SpinLock: the critical sections may be long (walk) and in
other places allocate memory. Base SWEB has no lock for the page tables at all, because a
process has only one thread – you add it the moment there can be two (A1).

## Pitfalls

- Forgetting `#include "types.h"` in `nonstd.h` → `unknown type name 'size_t'`.
- Forgetting the `break;` in the switch – falls through into the next case.
- Expecting `process_pages` to include the whole binary: pages are loaded **lazily** on the
  first access (`Loader::loadPage` from the page fault handler).
- Counting the page-table pages themselves, or the kernel half of the PML4.

## Answers to the tutor questions

- *Write directly?* Kernel and process share the address space (the kernel half is mapped in
  every PML4); the kernel runs with the process's `cr3`, so the user address is valid in the
  kernel too.
- *Unmapped page?* The kernel write page-faults in kernel mode on a user address →
  `checkPageFaultIsValid` accepts it → `loadPage`: loaded if a segment covers it, otherwise
  the process is killed (exit 666). Above `USER_BREAK`: we return -1 before touching it.
- *Why only 2 pages at the start?* Lazy loading: `UserProcess` maps only the stack page;
  every code/data page is loaded on its first page fault.
