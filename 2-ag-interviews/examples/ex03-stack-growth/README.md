# ex03 – a new case in the page fault handler: stack growth

Your team's code already grows the stack: a fault inside a thread's stack slot maps pages down to
the fault address, a fault on the guard page below kills the process. But a slot has only
`USERSTACK_PAGES` = **4 pages** (16 KiB): the test's 40 KiB local array runs past the guard page
and the process is killed (exit 666 – the address belongs to no stack and no segment).
This example walks through how your page fault handler grows the stack, and raises the limit to
16 pages. Test: `ex_pagefault.sweb` (2 checks).

## What the example changes

| File | Lines |
|---|---|
| `common/include/kernel/UserProcess.h` | +1 −1 |

The code below is the important part. The exact, complete change: `example.patch` in this folder, or `task example ex03` and then
`git -C ~/Documents/University/2026-fall/os/repos/sweb-ag show` (`q` quits).

## The path of a page fault

```
CPU: #PF (vector 14), cr2 = address, error code = flags
  → arch_pageFaultHandler (asm)       arch/x86/64/source/arch_interrupts.S
  → pageFaultHandler(address, error)  arch/x86/64/source/InterruptUtils.cpp: decode the flags
  → PageFaultHandler::enterPageFault  common/source/mm/PageFaultHandler.cpp: switch to kernel
                                      registers, interrupts ON
  → handlePageFault
      checkPageFaultIsValid?  no  → print, backtrace, Syscall::exit(9999)
                              yes → stack growth (thread_lock_) / guard page / loadPage(address, getAddressSpace())
                                    ← new cases go FIRST in this branch (heap, swap-in, COW, ...)
  ← back: arch_contextSwitch → the faulting instruction runs again
```

`checkPageFaultIsValid` rejects: null page (< 4096), kernel addresses, and *present* pages
(a write to a read-only page, …). Everything else is "valid": user address, not present.

## How your team's stack grows

Your team's code as it is (`common/source/mm/PageFaultHandler.cpp`, `handlePageFault`, lines 79–135 –
the example does not change it):

```cpp
UserThread* thread = static_cast<UserThread*>(currentThread);
assert(thread && "Thread is no UserThread");

UserProcess* current_process = currentThread->getProcess();
assert(current_process && "Thread has no process");

current_process->getThreadLock().acquire();

size_t fault_vpn = address / PAGE_SIZE;
size_t lowest_mapped_vpn = thread->getUserStackLowestMappedVpn();

if (isUserStackFault(thread, fault_vpn)) // check if fault is in range of threads stack TODO: maybe move into process class
{
  size_t current_lowest_vpn = lowest_mapped_vpn;
  while (current_lowest_vpn > fault_vpn) // allocate pages until sat
  {
    size_t new_stack_vpn = current_lowest_vpn - 1;

    size_t new_stack_ppn = 0;
    if (!current_process->allocateStackPage(&new_stack_ppn))
    {
      current_process->getThreadLock().release();

      debug(PAGEFAULT, "Could not allocate new page for stack of thread id: %zu", thread->getTID());
      Syscall::exit(9999);
    }

    if (!current_process->mapStackPage(new_stack_ppn, new_stack_vpn, 1))
    {
      current_process->getThreadLock().release();

      debug(PAGEFAULT, "Could not map new page for stack of thread id: %zu", thread->getTID());
      current_process->deallocateStackPage(new_stack_ppn);
      Syscall::exit(9999);
    }

    current_lowest_vpn = new_stack_vpn;
  }
  thread->setUserStackLowestMappedVpn(current_lowest_vpn);

  current_process->getThreadLock().release();

  return; // stack fault resolved
}
else if (current_process->isGuardPage(fault_vpn)) // check if we hit a guard page
{
  current_process->getThreadLock().release();

  debug(PAGEFAULT, "Well done, we hit a Guard page. Think about that\n");
  Syscall::exit(9999);
}
else
{
  current_process->getThreadLock().release();

  current_process->getLoader().loadPage(address, current_process->getAddressSpace());
}
```

Each thread gets a **slot** (`UserProcess::allocateNewStack`): `USERSTACK_PAGES` pages + 1 guard
page, counted down from `USER_BREAK`; only the top page is mapped at the start.

## The change

```cpp
// common/include/kernel/UserProcess.h
#define USERSTACK_PAGES 16   // was 4: a 40 KiB local array ran past the guard page
```

One line – because the mechanism is there. The slot stride (`USERSTACK_PAGES + GUARD_PAGE`) and
`getUserStackBottomVpn` follow automatically.

## Notes

- The handler runs with **interrupts enabled** in the context of the faulting thread – it
  may take Mutexes, allocate, even do disk I/O (A2 swap-in!).
- Faults from **kernel mode** on user addresses come here too (a syscall writing into a user
  buffer that is not mapped yet) – `user == false`, but the address is a user address.
- Never `Syscall::exit()` while holding a lock.
- If your handler returns **without** fixing the cause, the instruction faults again – forever:
  SWEB seems to hang with the same `[PAGEFAULT] Address: ...` line repeating.
  `InterruptUtils::countPageFault(address)` exists to catch that (halts after 10 identical
  faults) – add a call at the top of `pageFaultHandler` in `InterruptUtils.cpp` while debugging.
