# A1-11 · page fault statistics (the assembly entry)

**Time box 30 min · ★★☆**

```
Name:         pfstats
Nr:           1690
Description:  Count the page faults of each process: total, from user mode, from kernel
              mode, writes, reads, instruction fetches, and the last faulting address.
              Count them in the C entry of the page fault interrupt (pageFaultHandler in
              InterruptUtils.cpp). Verify there that the address the assembly stub passed
              is what the CPU's cr2 register holds (inline asm).
Parameters:   struct pfstats* stats
Return Value: 0 or -1
```

## Test

`task.sh start a1-11` → `task.sh test`. Done when 6 checks pass, including a fault the
*kernel* causes when the syscall writes into a not-yet-loaded user page.

## Hints

<details><summary>1 – the path of interrupt 14</summary>

`arch_pageFaultHandler` (arch/x86/64/source/arch_interrupts.S): saves the registers, reads
`cr2` into `rdi`, the error code from the stack into `rsi`, calls `pageFaultHandler(address,
error)` (InterruptUtils.cpp), which decodes the bits (P=1, W=2, U=4, I=16) and calls
`PageFaultHandler::enterPageFault`.
</details>

<details><summary>2 – locking: interrupts are OFF here</summary>

At that point interrupts are disabled (they are enabled later, in `enterPageFault`). A Mutex
would assert ("Lock ... with IF=0"). Use `ArchThreads::atomic_add` / `atomic_set` – or move
the counting after `enableInterrupts`.
</details>

<details><summary>3 – reading cr2</summary>

`asm volatile("mov %%cr2, %0" : "=r"(cr2));`
</details>

## Questions a tutor might ask

- Why could cr2 differ from the passed address later on?
- Which error code bits exist, and what does a kernel-mode fault on a user address mean?
- Why can you not take a Mutex in pageFaultHandler, but you can in handlePageFault?
