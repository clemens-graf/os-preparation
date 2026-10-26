# A1-11 · page fault statistics (the assembly entry)

**Time box 30 min · ★★☆**

**Theory first:** [concepts.pdf](../../concepts.pdf) §10 interrupts, §4 page faults

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

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

## Level 2 – hints

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

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Add a stats struct to `UserProcess`: total, user, kernel, write, read, instruction fetch, and the last faulting address – plain counters.
2. In `pageFaultHandler` (`InterruptUtils.cpp`, interrupts still off): read `cr2` with inline assembly and assert it equals the passed address.
3. Count with **atomic** additions (`ArchThreads::atomic_add`) – no Mutex here.
4. Decode the error code bits: user vs. kernel, write vs. read, present vs. not present.
5. Syscall `pfstats(stats)` (1690): check the user pointer, copy the counters (and the last faulting address) into the user's struct.
6. `task test`.

</details>

## Your team's code

The counters are per process → `UserProcess::PageFaultStats pf_stats_` (public), updated in `pageFaultHandler` (`InterruptUtils.cpp`) with `ArchThreads::atomic_add` – interrupts are off there, no lock possible. `currentThread->getProcess()` is `nullptr` for kernel threads: check it.

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
