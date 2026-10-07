# ex02 – map a page for user space

`scratchpage()` maps one page at the fixed address `0x700000000000`, pre-filled by the
kernel with "hello from the kernel". A second call returns the same page.
Test: `ex_mapping.sweb` (2 checks).

## The recipe

```cpp
size_t ppn = PageManager::instance()->allocPPN();               // 1. a physical page (zeroed)
char* kernel_view = (char*) ArchMemory::getIdentAddressOfPPN(ppn);
memcpy(kernel_view, greeting, sizeof(greeting));                 // 2. fill it via the ident mapping

loader->arch_memory_lock_.acquire();
bool mapped = loader->arch_memory_.mapPage(vpn, ppn, 1);         // 3. map it (user_access = 1)
loader->arch_memory_lock_.release();

if (!mapped)                                                     // 4. already mapped: give it back
  PageManager::instance()->freePPN(ppn);
```

- **Page numbers, not addresses:** `vpn = address / PAGE_SIZE`, `ppn` from the PageManager.
- **Ident mapping:** every physical page is visible to the kernel at
  `IDENT_MAPPING_START + ppn * PAGE_SIZE` – in every address space. So the kernel can fill a
  page before (or without) mapping it anywhere in user space.
- `mapPage` returns `false` if the vpn is already mapped – the caller still owns its `ppn`.
- `strcpy` is banned in the kernel (it asserts) – use `memcpy`/`strncpy`.

## The page-table lock

Base SWEB has **no lock** around the page tables: one thread per process, so nobody else
could change them. The moment a process has a second thread (A1) – or another kernel path
touches its tables – you need one. This example introduces it, and every solution uses it:

```cpp
// Loader.h
Mutex arch_memory_lock_;   // protects arch_memory_: every mapPage/unmapPage/PTE change
```

and wraps the existing `mapPage` in `Loader::loadPage` with it too. A Mutex (not a SpinLock):
the critical sections can be long and may allocate.

Lock order: `arch_memory_lock_` → PageManager's lock (taken inside `mapPage` when it needs a
new page table). Never the other way round.

## Traps

- **Initializer order:** `Loader`'s constructor must initialise members in declaration order
  (`-Werror=reorder`), so `arch_memory_lock_("...")` comes first in the list.
