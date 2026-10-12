# ex03 – a new case in the page fault handler: stack growth

> **Your team's code already grows the stack** (`isUserStackFault` in `PageFaultHandler.cpp`:
> a fault inside a thread's stack slot maps pages down to the fault address, a fault on the guard
> page below kills the process). So `task example ex03` runs on **base SWEB** (branch
> `base-sweb/example/…`). Read it to understand the mechanism, then find the same steps in your
> team's page fault handler – and be able to explain why your stacks have a guard page and at most
> `USERSTACK_PAGES` pages.

A process starts with one mapped stack page. Today a function with a 40 KiB local array
kills it (no segment covers the stack → exit 666). This example maps stack pages on demand:
any fault in the top 64 pages below `USER_BREAK` gets a fresh zero page.
Test: `ex_pagefault.sweb` (2 checks).

## The path of a page fault

```
CPU: #PF (vector 14), cr2 = address, error code = flags
  → arch_pageFaultHandler (asm)       arch/x86/64/source/arch_interrupts.S
  → pageFaultHandler(address, error)  arch/x86/64/source/InterruptUtils.cpp: decode the flags
  → PageFaultHandler::enterPageFault  common/source/mm/PageFaultHandler.cpp: switch to kernel
                                      registers, interrupts ON
  → handlePageFault
      checkPageFaultIsValid?  no  → print, backtrace, Syscall::exit(9999)
                              yes → currentThread->getLoader()->loadPage(address)   ← new cases go HERE, before it
  ← back: arch_contextSwitch → the faulting instruction runs again
```

`checkPageFaultIsValid` rejects: null page (< 4096), kernel addresses, and *present* pages
(a write to a read-only page, …). Everything else is "valid": user address, not present.

## The change

```cpp
if (checkPageFaultIsValid(address, user, present, switch_to_us))
{
  if (address >= STACK_BOTTOM)                     // USER_BREAK - 64 pages
    currentThread->getLoader()->mapZeroPage(address / PAGE_SIZE);
  else
    currentThread->getLoader()->loadPage(address);
}
```

`Loader::mapZeroPage(vpn)`: allocPPN outside the lock, `mapPage` under
`arch_memory_lock_`, free the ppn if `mapPage` returned false (another fault was faster).

## Notes

- The handler runs with **interrupts enabled** in the context of the faulting thread – it
  may take Mutexes, allocate, even do disk I/O (A2 swap-in!).
- Faults from **kernel mode** on user addresses come here too (a syscall writing into a user
  buffer that is not mapped yet) – `user == false`, but the address is a user address.
- Never `Syscall::exit()` while holding a lock.
- If your handler returns **without** fixing the cause, the instruction faults again – forever:
  SWEB seems to hang with the same `[PAGEFAULT] Address: ...` line repeating.
  `InterruptUtils::countPageFault(address)` exists to catch that (halts after 10 identical
  faults), but base SWEB never calls it – add a call at the top of `pageFaultHandler` in
  `InterruptUtils.cpp` while debugging.
