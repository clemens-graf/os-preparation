# A1-11 · page fault statistics (the assembly entry)

**Time box 30 min · ★★☆**

**Theory first:** [concepts.pdf](../../concepts.pdf) §10 interrupts, §4 page faults

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

## Your team's code

The counters are per process → `UserProcess::PageFaultStats pf_stats_` (public), updated in `pageFaultHandler` (`InterruptUtils.cpp`) with `ArchThreads::atomic_add` – interrupts are off there, no lock possible. `currentThread->getProcess()` is `nullptr` for kernel threads: check it.

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

Answer each one out loud first – then open it to compare.

<details><summary>Why could cr2 differ from the passed address later on?</summary>

`cr2` holds the address of the **most recent** page fault. If the handler re-enables interrupts and another thread runs and faults (or the handler itself faults, e.g. touching a user buffer), `cr2` is overwritten. That's why the assembly stub reads `cr2` **at entry** and passes it on as an argument.

</details>

<details><summary>Which error code bits exist, and what does a kernel-mode fault on a user address mean?</summary>

Bit 0 present (0 = not present, 1 = protection violation), bit 1 write (0 = read), bit 2 user (fault from user mode), bit 3 reserved bit set, bit 4 instruction fetch (NX). A kernel-mode fault on a user address = the kernel touched user memory for a syscall (e.g. `write`'s buffer) – legal: the page is loaded on demand, as for the user.

</details>

<details><summary>Why can you not take a Mutex in pageFaultHandler, but you can in handlePageFault?</summary>

`pageFaultHandler` runs right after the interrupt entry with **interrupts off** – a Mutex might sleep, which must not happen there. `PageFaultHandler::enterPageFault` switches to the thread's kernel context and **turns interrupts on**; from then on (`handlePageFault`) the thread is an ordinary kernel thread and may sleep on a Mutex.

</details>
