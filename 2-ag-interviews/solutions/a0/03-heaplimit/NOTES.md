# A0-03 heaplimit – solution notes

Patch for your team's code: [`team/solutions/a0/03-heaplimit/solution.patch`](../../../team/solutions/a0/03-heaplimit/solution.patch)
(`task solution`). Base SWEB (`task mode base`): [`solution.patch`](solution.patch) here.

## Files touched

| File | What |
|---|---|
| `UserProcess.h` | `HEAP_START`, `HEAP_MAX_SIZE`; `size_t heap_size_` (per process, under `thread_lock_`); `mapHeapPage(vpn)`, `setHeapLimit(size)` |
| `UserProcess.cpp` | init `heap_size_(0)` (initializer order = declaration order, `-Werror=reorder`!), the two methods |
| `PageFaultHandler.cpp` | new branch before `loadPage` for heap addresses |
| `Syscall.h/.cpp`, `syscall-definitions.h` | `sc_heaplimit 1520`, `heapLimit(size)` → `currentThread->getProcess()->setHeapLimit(size)` |
| `nonstd.h/.c` | `HEAP_START` for user space, wrapper |

## The page fault side

First in the valid branch of `handlePageFault` – before your team's code takes `thread_lock_`
for stack growth (`mapHeapPage` takes it itself):

```cpp
if (address >= HEAP_START && address < HEAP_START + HEAP_MAX_SIZE)
{
  if (!currentThread->getProcess()->mapHeapPage(address / PAGE_SIZE))
    Syscall::exit(9998);                     // above the limit
  return;
}

bool UserProcess::mapHeapPage(size_t vpn)   // per-process state: UserProcess
{
  size_t ppn = PageManager::instance()->allocPPN();
  thread_lock_.acquire();
  bool inside = vpn * PAGE_SIZE < HEAP_START + heap_size_;
  bool mapped = inside && arch_memory_->mapPage(vpn, ppn, 1);
  thread_lock_.release();
  if (!mapped)
    PageManager::instance()->freePPN(ppn);
  return inside;
}
```

## The syscall side

Round up to pages, then `currentThread->getProcess()->setHeapLimit(size)`: under the lock,
unmap every mapped heap page between the new and the old limit, flush the TLB, store the new
size.

## Locking

- `heap_size_` and the page tables are protected by **the same** lock. That is what makes
  "check the limit + map" atomic with respect to `heapLimit`.
- **The bug to avoid** (it was in my first version): read the limit under the lock,
  release, then map. `heapLimit` shrinking the heap in between leaves a page mapped above
  the limit that nobody will ever unmap until exit.
- Two threads fault on the same heap page: the second `mapPage` returns false → it frees
  its physical page and returns `true` (inside) – both continue, the page is mapped once.
- `Syscall::exit` is called **after** the lock is released. A thread that dies while holding
  a mutex blocks every other thread of the process forever (and `~Thread` asserts).
- Why `UserProcess` and not `Loader`: the limit belongs to the process; the `Loader` is about the
  program file and the page tables. The lock is still the page-table lock, because limit and
  mappings must change together.

## Pitfalls

- Hooking into `checkPageFaultIsValid` instead of the valid branch: then null pointers etc.
  would also have to be considered.
- `heap_size_` added to the class but not to the constructor's initializer list in the
  right order → `-Werror=reorder`.
- Forgetting the TLB flush when shrinking.

## Answers to the tutor questions

- *First write to HEAP_START + 5000:* CPU raises #PF (not present, user, write) →
  `arch_pageFaultHandler` (asm) → `pageFaultHandler` (InterruptUtils.cpp) →
  `PageFaultHandler::enterPageFault` (interrupts on, kernel registers) → `handlePageFault`
  → heap branch → `mapHeapPage` → return → `arch_contextSwitch` → the write is repeated.
- *Grow automatically:* map on demand whenever the address is in the region, no limit
  check – that is how ex03 grows the stack.
