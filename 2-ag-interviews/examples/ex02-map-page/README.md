# ex02 – map a page for user space

`scratchpage()` maps one page at the fixed address `0x700000000000`, pre-filled by the
kernel with "hello from the kernel". A second call returns the same page.
Test: `ex_mapping.sweb` (2 checks).

## The recipe

```cpp
size_t ppn = PageManager::instance()->allocPPN();               // 1. a physical page (zeroed)
char* kernel_view = (char*) ArchMemory::getIdentAddressOfPPN(ppn);
memcpy(kernel_view, greeting, sizeof(greeting));                 // 2. fill it via the ident mapping

process->getThreadLock().acquire();
bool mapped = process->getAddressSpace().mapPage(vpn, ppn, 1);         // 3. map it (user_access = 1)
process->getThreadLock().release();

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

Two threads of a process must not change its page tables at the same time. In your team's code
that lock already exists: **`process->getThreadLock()`**, the per-process Mutex your page fault
handler takes when it maps stack pages. Every A0 solution uses it around `mapPage`/`unmapPage`:

```cpp
UserProcess* process = currentThread->getProcess();
process->getThreadLock().acquire();
bool mapped = process->getAddressSpace().mapPage(vpn, ppn, 1);
process->getThreadLock().release();
```

A Mutex (not a SpinLock): the critical sections can be long and may allocate. Lock order:
`thread_lock_` → PageManager's lock (taken inside `mapPage` when it needs a new page table).
Never the other way round.

(Base SWEB has no such lock – one thread per process. There the course adds
`Mutex arch_memory_lock_` to the `Loader`, initialised first in its constructor
because of `-Werror=reorder`.)

## Traps

- **Initializer order** (base SWEB only, when you add a lock to the `Loader`): members are
  initialised in declaration order (`-Werror=reorder`).
