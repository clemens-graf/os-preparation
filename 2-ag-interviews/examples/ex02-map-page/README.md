# ex02 – map a page for user space

`scratchpage()` maps one page at the fixed address `0x700000000000`, pre-filled by the
kernel with "hello from the kernel". A second call returns the same page.
Test: `ex_mapping.sweb` (2 checks).

## What the example changes

| File | Lines |
|---|---|
| `common/include/kernel/Syscall.h` | +3 |
| `common/include/kernel/syscall-definitions.h` | +3 |
| `common/source/kernel/Syscall.cpp` | +34 |
| `userspace/libc/include/nonstd.h` | +5 |
| `userspace/libc/src/nonstd.c` | +5 |

The code below is the important part. The exact, complete change: `example.patch` in this folder, or `task example ex02` and then
`git -C ~/Documents/University/2026-fall/os/repos/sweb-ag show` (`q` quits).

## The recipe

*Shortened* – the core of `Syscall::scratchPage()` in `Syscall.cpp` (the full function: `example.patch`):

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

## Traps

- **Initializer order** (when you add a Mutex/Condition member to a class, e.g. A1-03): members are
  initialised in declaration order (`-Werror=reorder`).
