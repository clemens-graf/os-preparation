# A0-12 two addresses, one page – solution notes

Patch: [`solution.patch`](solution.patch) (against upstream `f6fcb2ab`, tests excluded).
Starting point: the tutorial version, [ex12](../../../examples/ex12-tutorial2/).

## Pieces

| Piece | Where |
|---|---|
| syscall numbers 1560/1561 | `syscall-definitions.h` |
| `getPhysicalAddress`, `mapAddress` + two `case`s | `Syscall.h`, `Syscall.cpp` |
| `mapsPhysicalPage(ppn)`: walk the user half | `ArchMemory` |
| `unmapPage(vpn, free_physical_page = true)` | `ArchMemory` |
| `arch_memory_lock_` (ex02) + `alias_vpns_` | `Loader.h`, constructor, `loadPage` |
| unmap the aliases without freeing | `Loader::~Loader` |
| wrappers, `#include "types.h"` | `nonstd.h`, `nonstd.c` |

## The two syscalls

```cpp
size_t Syscall::getPhysicalAddress(size_t address)
{
  if (address >= USER_BREAK)
    return (size_t) -1;
  Loader* loader = currentThread->loader_;
  loader->arch_memory_lock_.acquire();
  ArchMemoryMapping m = loader->arch_memory_.resolveMapping(address / PAGE_SIZE);
  size_t ppn = (m.page_size == PAGE_SIZE) ? m.page_ppn : (size_t) -1;
  loader->arch_memory_lock_.release();
  return ppn;
}

size_t Syscall::mapAddress(size_t address, size_t ppn)
{
  if (address >= USER_BREAK)
    return (size_t) -1;
  size_t vpn = address / PAGE_SIZE;
  Loader* loader = currentThread->loader_;
  loader->arch_memory_lock_.acquire();
  bool mapped = loader->arch_memory_.mapsPhysicalPage(ppn)    // only frames of this process
                && loader->arch_memory_.mapPage(vpn, ppn, 1);   // false: vpn already mapped
  if (mapped)
    loader->alias_vpns_.push_back(vpn);                         // ~Loader unmaps it, no free
  loader->arch_memory_lock_.release();
  return mapped ? 0 : (size_t) -1;
}
```

`~Loader`, before anything else:

```cpp
for (size_t vpn : alias_vpns_)
  arch_memory_.unmapPage(vpn, false);       // the frame is still mapped at its original place
```

## Why each step

- **`address >= USER_BREAK` → -1:** the upper half of the PML4 is the kernel's. The tutorial
  version walked into it: `map_address` onto kernel text hit `assert(mapped)`.
- **`page_size == PAGE_SIZE`:** `resolveMapping` returns `page_ppn = 0` for a page that is not
  mapped – and 0 is a valid frame number. An untouched page (lazy loading!) is "not mapped".
- **The ownership walk:** without it a process can map any frame – another process's page, the
  kernel's code (frame 256 in the test), page tables – writable. It is the security hole of the
  tutorial version. There is no reverse map, so the walk over all present user PTEs is the
  honest way (the same loops as `~ArchMemory` and A1-09's `ptov`).
- **No `assert`:** `mapPage` returning `false` is a user error (the address is in use), not a
  kernel bug. An assert there lets any program crash the kernel.
- **The alias list + unmap without free:** `~ArchMemory` frees *every* present user page.
  An alias frame is present twice → `freePPN` twice → `"Double free PPN"` panic, in the
  CleanupThread (which runs the destructors). `Loader::~Loader`'s body runs before the members
  are destroyed, so it can remove the aliases first. The original mapping keeps the frame and
  frees it exactly once.
- **No TLB flush** after `mapPage`: the entry was not present, so no stale translation can be
  cached. None in `~Loader` either: that address space never runs again (the CleanupThread runs
  on another `cr3` – `~ArchMemory` even asserts it).

## Locking

One lock: `Loader::arch_memory_lock_` (Mutex), the page-table lock of ex02. It protects the
page tables and `alias_vpns_`. Critical sections:

- `getPhysicalAddress`: the `resolveMapping` (another thread could unmap the page while the
  walk reads the tables);
- `mapAddress`: ownership walk **+** `mapPage` **+** `push_back` – one section. Released in
  between, another thread of the process could unmap and free the frame after the check (with
  a syscall like A0-02's `freepage`), and the new alias would point to a frame that now belongs
  to somebody else;
- `Loader::loadPage`: its `mapPage` (the page fault path changes the same tables).

`~Loader` takes no lock: no thread of the process exists any more. Lock order:
`arch_memory_lock_` → PageManager's lock (taken inside `mapPage` for new page tables) → the
kernel heap lock (`push_back`). Never the other way round.

## Pitfalls

- `-Werror=reorder`: `arch_memory_lock_` and `alias_vpns_` are declared after `arch_memory_`
  and before `fd_` – the initializer list must follow that order.
- `nonstd.h` does not include `types.h` – without it `size_t` is unknown in user programs.
- Returning `-1U` instead of `(size_t) -1`: user space then sees `0xFFFFFFFF`, not -1.
- Forgetting the stack: `alias_basic` also aliases the stack page – it is freed by `~ArchMemory`
  too, so the same rule applies.
- `ustl::list` is a vector in uSTL: `push_back` may allocate – fine under a Mutex, never with
  interrupts off.

## Answers to the card's questions

- **Panic in the tutorial version:** see "Why each step"; CleanupThread because it deletes the
  dead thread and with it the `UserProcess` → `Loader` → `ArchMemory`.
- **0:** a valid frame number. `&global` fails before the first access because SWEB maps pages
  lazily – the page fault on the first access maps it.
- **Any ppn:** read and write any physical memory – other processes, the kernel, page tables.
- **One critical section:** check-then-act (TOCTOU) – the frame could be freed and reused in
  between.
- **`(vpn << 12) + offset`:** the page table translates only the upper bits (the page number);
  the low 12 bits are the position inside the page and pass through unchanged.
- **TLB:** see "Why each step".
- **`freepage`:** it frees a frame that may still be mapped as an alias somewhere. You would
  need a reference count per frame (or: forbid unmapping a frame that has aliases), as A1-10
  does for its shared pages.
