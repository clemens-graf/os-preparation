# Triage – an unknown task in 5 minutes

In the interview you get a task you have never seen. Before writing a line: **what kind of
task is it, and where does the code go?** Every SWEB task is one of five types (or two of them
combined). This page tells you how to recognise the type and where to start.
File details: [ATLAS.md](ATLAS.md).

## The five types

| Type | Recognise it by | The code goes to | Example tasks |
|---|---|---|---|
| **U – user space only** | a standard (POSIX/C) function with a man page whose stub exists in the libc; needs nothing only the kernel knows | `userspace/libc/src/*.c` (+ maybe its header) – **no syscall, no kernel** | `atexit`, `malloc`, spinlock, semaphore |
| **S – syscall + kernel** | "a syscall that …", or the function needs kernel data (page tables, threads, other processes, devices) | the six places + kernel logic in `Syscall.cpp` / `UserProcess` / `ArchMemory` | meminfo, allocpage, pthread_join, vtop, shared page |
| **E – kernel event** | "when X happens …": a page fault, a timer tick, scheduling, a key press, boot, process exit, a kernel thread | the handler of that event (table below) – often **no new syscall** (or one only for testing) | heap on demand, swap, F3, NX, boot counter, scheduler change, RAM obfuscation |
| **A – assembly / registers** | many arguments, "in assembly", register contents, the interrupt entry, saving/restoring a thread's state | `arch/x86/64/source/arch_interrupts.S`, `ArchThreads`, `user_registers_` | sum15, page-fault stats from the entry, snapshot/revive |
| **K – kernel calls user code** | "the kernel calls a user function", "handler", "callback", signal-like | change `user_registers_->rip/rsp/rdi` so the thread continues in a user function, then comes back | upcall, segv handler, kernel-side atexit |

### The events of type E – where each one is handled

| Event | Handler |
|---|---|
| page fault (lazy loading, heap, stack, swap-in, copy-on-write, protection) | `PageFaultHandler::handlePageFault` (`common/source/mm/PageFaultHandler.cpp`) |
| loading the program (flags of code/data pages, NX, zero pages) | `Loader::loadPage` (`common/source/kernel/Loader.cpp`) |
| timer tick (count, measure time) | `irqHandler_0` (`InterruptUtils.cpp`) / `Scheduler::incTicks` |
| choosing the next thread | `Scheduler::schedule` (`Scheduler.cpp`) |
| key press | `Console::handleKey` (`Console.cpp`) – set a flag, act at the top of `syscallException` |
| boot | `startup()` in `main.cpp` |
| process ends | `Syscall::exit` → `UserProcess::exitProcess`; cleanup in `~UserProcess` |
| thread ends | `UserProcess::exitThread`, `~UserThread` |
| every syscall (blacklist, cancellation point, F-key flag) | top of `Syscall::syscallException` |
| "in the background" | a new kernel thread: class with `Run()`, `addNewThread` in `main.cpp` |

## The first five minutes – the same every time

1. **Read the box twice.** Underline: names, numbers, return values, error cases, "when …".
2. **Type?** Use the table above. Not sure between U and S? Ask: *does it need anything that
   only the kernel has?* (other processes, page tables, physical pages, devices, the
   scheduler, the time). No → U.
3. **Find the starting point** – one search:
   - U: `git grep -n "malloc" userspace/libc` → the stub
   - S: `syscall-definitions.h` (pick a free number in your block)
   - E: the handler from the table
   - A: `arch_interrupts.S` / `ArchThreads.h`
   - K: `user_registers_` – where is a thread's `rip` set? (`ArchThreads::changeInstructionPointer`)
4. **Where does the state live?** Per process → `UserProcess.h`; per thread → `UserThread.h`;
   per page → a free bit in the page-table entry or a list in the process; whole kernel →
   a static member / global; user side → a `static` variable in the libc file.
5. **Who else touches it at the same time?** Other threads of the process → `getThreadLock()`;
   the page fault handler → the same lock; a key handler / timer → a flag (no lock in an interrupt).
6. **Write the file list down** (3–6 files), then: plumbing → build → logic → test.

Say these six steps out loud in the interview – it shows the tutor you know your way around.

## Patterns per type

### U – fill in a libc stub
```c
// userspace/libc/src/stdlib.c
static void (*handlers[32])(void);   // state: static in the libc file (one copy per process)
static int count = 0;

int atexit(void (*function)(void))
{
  if (function == 0 || count == 32)
    return -1;
  handlers[count++] = function;
  return 0;
}
```
- The header usually already declares it – check `userspace/libc/include/*.h`.
- State is `static` in the `.c` file: every process has its own copy (own address space).
- Test: a program in `userspace/tests/` that calls it and prints `[PASS]`/`[FAIL]`.
- Threads: is it called by several threads at once? Counters for registration (atexit) are
  fine without; **locks themselves** need one atomic instruction (A1-15).

### S – the six places
`syscall-definitions.h` → `Syscall.h` → `Syscall.cpp` (case + function) → `nonstd.h` →
`nonstd.c` → test. Check every user pointer first. State in `UserProcess`/`UserThread`.

### E – hook into the handler
- Put your case **first** in the handler and `return` when you handled it.
- Never `Syscall::exit` while holding a lock.
- Key/timer: interrupts are off or you are in another thread → only set a flag; the work
  happens in the thread's own context (top of `syscallException`, ex07).
- Test: trigger the event from a user program (touch a page, loop for ticks, `!key f3` in
  the test command).

### A – registers and assembly
- The saved user registers: `currentThread->user_registers_` (`ArchThreadRegisters`: `rip`,
  `rsp`, `rax`, `rbx`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`–`r15`, …).
- Syscall arguments: `rbx rcx rdx rsi rdi` (+ more registers or the user stack for more args).
- Snapshot = copy `*user_registers_` (and the stack page); revive = copy back.

### K – make the thread continue in a user function
```cpp
ArchThreadRegisters* r = currentThread->user_registers_;
size_t rsp = r->rsp - 128;        // skip the red zone
rsp = rsp - rsp % 16;             // 16-byte alignment
rsp = rsp - sizeof(size_t);       // room for 1 value
pointer slot = process->getAddressSpace().checkAddressValid(rsp);   // mapped? (under getThreadLock())
if (!slot) return (size_t) -1;
*(size_t*) slot = r->rip;     // "push" the old rip as return address (via the kernel's mapping)
r->rsp = rsp;
r->rip = handler;             // the thread continues in the handler ...
r->rdi = argument;            // ... with its first argument
```
The handler's `ret` jumps back to the old `rip` (A0-07). In the kernel, write user memory
through `checkAddressValid` (it returns the kernel address of that byte), not through the user pointer.

## Every course task by type

| Task | Type | What it trains |
|---|---|---|
| A0-01 meminfo | S | user struct, page-table walk |
| A0-02 allocpage | S | allocPPN + mapPage / unmapPage + TLB |
| A0-03 heaplimit | S + E (page fault) | lazy mapping in the page fault handler |
| A0-04 magic page | E (page fault) + S | read-only page, special address |
| A0-05 readonly code | E (loader) | page flags when loading |
| A0-06 segv handler | K + E (page fault) | call a user function on a fault |
| A0-07 upcall | K | rip/rsp/rdi manipulation |
| A0-08 pthread_create | S | stack, registers, address space |
| A0-09 pthread_multi | S + K | thread runs a list of functions |
| A0-11 thread note | S | user string, per-thread data, debug flag |
| A0-12 alias page | S | resolveMapping, two mappings, cleanup |
| A0-13 atexit | **U** | libc only: exit() calls the handlers |
| A0-14 string functions | **U** | libc only: missing functions, pure C |
| A1-01 pthread_join | S | blocking with a Condition |
| A1-02 pthread_multiple | S | many threads, join_any |
| A1-03 barrier | S | blocking for N threads |
| A1-04 thread flag | E (scheduler) + S | change `schedule()` |
| A1-05 pthread_invoke | E (scheduler) + S | pick the next thread |
| A1-06 pthread_cancel | S + E (syscall entry) | deferred cancellation |
| A1-07 thread runtime | E (timer) + S | count ticks per thread |
| A1-08 snapshot | A + S | save/restore registers and stack |
| A1-09 vtop / ptov | S | page tables in both directions |
| A1-10 shared page | S + E (exit) | one page in two processes, reference count |
| A1-11 pagefault stats | A + E | the assembly entry, count per process |
| A1-12 sbrk | S + E (page fault) | per-process break, on demand |
| A1-13 sum15 | A | 15 arguments through registers + stack |
| A1-14 malloc | **U** | libc only: a heap on a static pool |
| A1-15 spinlock + mutex | **U** | libc only: the one atomic you need |
| A1-16 semaphore | **U** | libc only: built on A1-15 |
| A1-17 condition variables | **U** | libc only: wait / signal on top of A1-15 |
| A1-18 sleep + uptime | S + E (timer) | the ticks of the timer interrupt |
| A1-19 waitpid | S | processes: exit codes that outlive the process |
| A1-20 pthread_detach | S | threads: free the record at the end |
| A1-21 forkall | S | processes + threads: copy every thread record and thread |
| A1-22 pthread_switch | **A** + U | registers: the context switch itself, in user space |
| A1-23 pthread_switch (ids) | S | threads: swap the ids in record + thread object |
| A2-01 swap | S + E (page fault) | block device, swap-out/in |
| A2-02 page backup | S | copy a page to the disk and back |
| A2-03 heap swap | E (page fault) | swap to the kernel heap |
| A2-04 xor swap | E | encrypt on the way out |
| A2-05 dirty swap | E | the dirty bit |
| A2-06 F3 swap all | E (key) + S | flag in the console, work in the process |
| A2-07 mprotect | S + E (page fault) | page flags from user space |
| A2-08 NX | E (loader) | execution_disabled bit |
| A2-09 zero page | E (loader + page fault) | sharing + copy-on-write |
| A2-10 RAM obfuscation | E (kernel thread) | a kernel thread moving pages |
| A2-11 boot counter | E (boot) + S | block device at boot |
| A2-12 syscall blacklist | E (syscall entry) + S | per-process state checked on every syscall |
| A2-13 anonymous mmap | S + E (page fault) | regions in the process, mapped on demand |
