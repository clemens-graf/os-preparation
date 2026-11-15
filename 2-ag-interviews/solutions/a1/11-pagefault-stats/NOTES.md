# A1-11 page fault statistics – solution notes

Patch: [`solution.patch`](solution.patch).

## The path

```
CPU: #PF, cr2 = address, error code pushed
arch_pageFaultHandler (arch_interrupts.S):  pushAll; arch_saveThreadRegisters;
                                            movq %cr2, %rdi; movq 144(%rsp), %rsi (error code)
pageFaultHandler(address, error)  (InterruptUtils.cpp)   <- interrupts still OFF here
PageFaultHandler::enterPageFault                          <- enables interrupts
```

## The counting

```cpp
uint64 cr2;
asm volatile("mov %%cr2, %0" : "=r"(cr2));
assert(cr2 == address);                       // nothing could have overwritten it yet

if (currentThread->getProcess()) {
  UserProcess::PageFaultStats& s = currentThread->getProcess()->pf_stats_;
  ArchThreads::atomic_add(s.total, 1);
  ArchThreads::atomic_add((error & FLAG_PF_USER) ? s.user : s.kernel, 1);
  ...
}
```

## Locking

Interrupts are off: a Mutex asserts (`Lock ... with IF=0 ... Now we're dead`). Atomic
instructions need no lock. Alternative: count in `handlePageFault` (interrupts on) under a Mutex.

## Why cr2 can change

Once interrupts are enabled, another page fault (of another thread, or a kernel page fault while
handling this one) overwrites cr2. That is why the stub reads it immediately.

## Error code bits

P (1: protection violation, 0: not present), W (write), U (user mode), RSVD, I (instruction fetch).
A kernel-mode fault on a user address = a syscall touched a user page that is not loaded yet.

## Shortest working version (tested)

[`short.patch`](short.patch) – **48 lines** instead of 63, passes the same test. Get it with
`task short a1-11` (branch `short/…`, then `task test`).

What is shorter: an array `size_t pf_stats_[7]` instead of a struct; plain `++` instead of atomic adds (fine on one CPU – the tutor may ask about atomic).

Use it to see the **core** of the task. In the interview, the full solution's extra checks are what
a tutor asks about ("what if the pointer is invalid / two threads at once?").
