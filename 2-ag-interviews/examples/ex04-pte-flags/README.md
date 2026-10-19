# ex04 – change a page table entry: read-only pages

`protectpage(address, writable)` makes an existing mapping read-only or writable again.
Test: `ex_pte_flags.sweb` (4 checks, then the process must be killed by its write).

## What the example changes

| # | File | Change |
|---|---|---|
| 1 | `common/include/kernel/syscall-definitions.h` | `#define sc_protectpage 1420` |
| 2 | `common/include/kernel/Syscall.h` | `static size_t protectPage(size_t address, size_t writable);` |
| 3 | `common/source/kernel/Syscall.cpp` | `#include "ScopeLock.h"` (+ `ArchMemory.h`, `Loader.h` – not strictly needed) and `case sc_protectpage:` in `syscallException` |
| 4 | `common/source/kernel/Syscall.cpp` | the function below |
| 5 | `userspace/libc/include/nonstd.h` | `#include "types.h"`, `extern int protectpage(void* address, int writable);` |
| 6 | `userspace/libc/src/nonstd.c` | `return __syscall(sc_protectpage, (size_t) address, writable, 0x00, 0x00, 0x00);` |

Places 1–3, 5, 6 are the usual syscall plumbing (ex01). The interesting part is place 4 –
the whole function, exactly as in `example.patch`:

```cpp
size_t Syscall::protectPage(size_t address, size_t writable)
{
  if (address >= USER_BREAK)                   // never let user space touch kernel mappings
    return (size_t) -1;

  UserProcess* process = currentThread->getProcess();
  ScopeLock lock(process->getThreadLock());   // released automatically at every return

  ArchMemoryMapping m = process->getAddressSpace().resolveMapping(address / PAGE_SIZE);
  if (m.page_size != PAGE_SIZE)                // not mapped (yet) - m.pt may even be null
    return (size_t) -1;

  m.pt[m.pti].writeable = writable ? 1 : 0;    // the one bit this syscall is about
  ArchMemory::flushTlb();                      // the CPU may still cache the old entry in its TLB
  return 0;
}
```

See it yourself: `task example ex04`, then
`git -C ~/Documents/University/2026-fall/os/repos/sweb-ag show` (the full change, `q` to quit).

## Watch it in the debugger

Breakpoints at the first line of `protectPage` and at `PageFaultHandler.cpp:38` (the
"mapped" branch of `checkPageFaultIsValid`), then run `ex_pte_flags.sweb`:

| Stop | `writable` | What you see |
|---|---|---|
| 1 | 0 | `m.page_size` = 0 → `return -1`: the page is not loaded yet (lazy loading) |
| 2 | 0 | `m.page_size` = 4096; watch `m.pt[m.pti]`: `writeable` 1 → 0 after the assignment |
| 3 | 1 | back to 1 |
| 4 | 0 | read-only again |
| 5 | – | the page fault of `page[2] = 'z'`: `present` = true, `user` = true → exit 9999 |

## Background

- `resolveMapping(vpn)` walks the 4 levels and returns pointers (ident addresses) to every
  level: `m.pml4`, `m.pdpt`, `m.pd`, `m.pt` and the indices `m.pti` etc. A level that does
  not exist is `nullptr`; `m.page_size` is `PAGE_SIZE` only if the 4 KiB page is present.
- The PTE fields (`paging-definitions.h`, `PageTableEntry`): `present`, `writeable`,
  `user_access`, `accessed`, `dirty`, `page_ppn`, `execution_disabled` (NX) – and
  `ignored_1` (11 bits) / `ignored_2` (3 bits) that the CPU ignores: free for your own
  flags (A2: "this page is swapped out, slot N").
- **TLB flush** after *removing* rights: the CPU may still have the old (writable) entry
  cached. Adding rights or mapping a non-present page needs no flush (a missing/weaker
  cached entry just faults once more).

## The write that kills

A write to a present read-only page raises a #PF with the *present* flag set →
`checkPageFaultIsValid`: "You got a pagefault even though the address is mapped" → exit 9999.
