# A0-08 · pthread_create – the mechanics

**Time box 60 min · ★★★ · type S ([TRIAGE](../../TRIAGE.md)) · read first: [ex06](../../examples/ex06-kernel-thread/), [ex02](../../examples/ex02-map-page/)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §15 a thread's life, §8 registers & calling convention

```
Name:         pthread_create, pthread_exit (minimal)
Nr:           1550 (sc_pthread_create), 1551 (sc_pthread_exit)
Description:  pthread_create starts a new thread in the calling process that runs
              start_routine(arg) on its own user stack, in the same address space.
              When start_routine returns, the thread ends (libc calls pthread_exit).
              pthread_exit ends the calling thread; called by the main thread it behaves
              like exit().
              Returning from main is exit(): the whole process ends. If main calls
              pthread_exit instead, the process (its address space) stays alive until
              all its other threads are gone.
Parameters:   pthread_t* thread (receives an id > 0, unique in the process),
              const pthread_attr_t* attr (ignored), void* (*start_routine)(void*), void* arg
Return Value: 0, or -1 (bad pointers, no more threads possible)
Notes:        Simplifications for A0: no pthread_join, exit() from a thread other than
              main ends only that thread, at most 64 threads per process lifetime.
              Each thread gets a stack of 4 pages.
```

## Test

`task start a0-08`, implement, `task test` (runs `pthread_create_basic.sweb`, `pthread_create_exit.sweb`, then `exit`).
Done when 9 checks pass, *"thread finished after main called pthread_exit"* appears, and after
`exit` there is no `leaking` warning (all stacks and page tables freed).

## Your team's code

The task starts from your team's `main` (`osw26e3`, the commit in `team/BASE`) – the code you have in the interview.

**In your team's code `pthread_create` already exists** (Michael's work, merged), so here the
task is to **read and explain it**, not to build it. `task start a0-08` gives you `main` plus the
course tests, and `task test` runs them against your team's implementation: all 9 checks pass.
Note the two ways a main thread can end: returning from `main` calls `exit()` (`_start`), and
`exitProcess` kills all threads – POSIX; `pthread_exit` in `main` ends only the main thread, and
the process lives on until its last thread is gone (`pthread_create_exit.sweb` checks that).

Find and explain, as if the tutor asked:
- `Syscall::pthreadCreate` → `UserProcess::createThread(entry, true, start_routine, arg)`: which
  checks, which lock (`thread_lock_`), when is the thread added to the scheduler?
- `UserProcess::allocateNewStack` / `allocateNewStackSlotNumber`: where do the stacks go, why a
  **guard page**, how big is a stack (`USERSTACK_PAGES`)?
- The `UserThread` constructor: `createUserRegisters`, `setAddressSpace`, `setThreadArguments` –
  which registers, and why does the thread start in libc's `pthread_start`?
- The page fault handler: how does a thread's stack **grow** (`isUserStackFault`)?
- `exitThread` / `exitProcess` / `UserThreadData`: who frees the stack, the thread object, the
  process? (Your join builds on exactly this.)
- What is shared between the threads, what is private?


## Hints

<details><summary>1 – what IS a thread in SWEB?</summary>

A `Thread` object = a kernel stack + kernel registers + (for user threads) user registers
+ a process (`getProcess()`, whose loader is the address space) + an entry in the scheduler's
list. In your team's code a user thread is a `UserThread`. Look at its constructor: it maps one
stack page, creates user registers (`ArchThreads::createUserRegisters`), sets the address space
(`ArchThreads::setAddressSpace`); `ProcessRegistry::createProcess` adds it to the scheduler. A
new thread needs the same steps – with the **same** address space (`getAddressSpace()`) and a **different** stack.
</details>

<details><summary>2 – where does the new thread start?</summary>

`rip` = a libc function `start(start_routine, arg)` that calls `start_routine(arg)` and then
`pthread_exit(result)` – that is how "returning ends the thread" works. The kernel gets its
address as an extra syscall argument. `rdi`/`rsi` carry the two arguments, `rsp` = the top of
its new stack minus 8.
</details>

<details><summary>3 – where does the stack go?</summary>

The main stack is the top page below `USER_BREAK`. Give every thread a slot: slot k
covers `USER_BREAK - (k+1)*SLOT .. USER_BREAK - k*SLOT`, map the top 4 pages of it. Unmap
them when the thread is destroyed (its destructor runs in the CleanupThread, when it can
never run again).
</details>

<details><summary>4 – lifetime: who frees the address space?</summary>

`UserProcess::~UserProcess` deletes its `loader_` (and with it the page tables), and your
team's `~UserThread` deletes the process. If any thread deletes it while others still run,
they run on freed page tables. Count the threads of the
process; the main thread's `exit` waits (Mutex + Condition) until the count is 0; the
destructor of each extra thread decrements it **as its very last action**.
</details>

<details><summary>5 – locking – three things</summary>

1. Now two threads share one `ArchMemory`: page faults of both map pages concurrently,
   `mapPage` allocates page-table pages – without a page-table lock, one of them gets lost.
   Add the lock and take it around **every** `mapPage`/`unmapPage` (also in `Loader::loadPage`).
2. The thread counter + stack slot counter: one Mutex, Condition for "count reached 0",
   `while`-loop around `wait`.
3. Do not create the thread object (which takes the page-table lock) while holding the
   counter lock – no nested locks if you do not need them.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Which registers did you set for the new thread and why exactly those?</summary>

In the `UserThread` constructor (`createUserRegisters` + `setThreadArguments` + `setAddressSpace`): `rip` = entry (the libc trampoline `pthread_start`), `rsp` = top of the thread's own stack slot, `rdi`/`rsi` = `start_routine` and `arg` (the trampoline's two arguments), user code/data segments and `rflags` with interrupts on, `cr3` = the process's page tables, `rsp0` = the thread's kernel stack (used at the next interrupt). Everything else starts at 0.

</details>

<details><summary>Why does the thread not start at <code>start_routine</code> directly?</summary>

Because `start_routine` may simply **return** – and on a fresh stack there is no return address to return to. The libc trampoline `pthread_start(start_routine, arg)` calls it and then `pthread_exit(result)`, so a return ends the thread properly.

</details>

<details><summary>What is shared between the threads, what is private?</summary>

**Shared** (the `UserProcess`): the address space – code, globals, heap, all stacks are visible to all –, the loader, open files, the working directory. **Private:** registers, the own user stack slot, the own kernel stack, the `UserThreadData` record.

</details>

<details><summary>When exactly is the stack of a finished thread unmapped – and why not in <code>pthread_exit</code>?</summary>

In your team's code `exitThread` frees the stack pages (`freeUserThreadStack`) under `thread_lock_`, while the thread still runs on its **kernel** stack – that's safe because it no longer uses the user stack. What must *not* happen inside `pthread_exit` is freeing the thread **object** or its kernel stack: the thread is still running on it. That's why the object is deleted later by the CleanupThread, after `kill()`.

</details>

<details><summary>What happens if <code>thread</code> points to an unmapped page? (order of operations in your syscall!)</summary>

Your team's `pthreadCreate` checks `thread < USER_BREAK` up front and writes `*thread = tid` **after** the thread was created but **before** `addNewThread`. If the page isn't loaded, the kernel write page-faults and `loadPage` maps it; if no segment covers it, the process dies (666) – with a created but never scheduled thread. Safer order: copy-out first (or check it is writable), then create.

</details>

<details><summary>What changes for <code>pthread_join</code> (A1)?</summary>

The thread's exit value must outlive the thread object (the CleanupThread deletes it at some point) – so a record per thread (`UserThreadData`: tid, exit value, exited, joined) stays until a join takes it. `exitThread` stores the value and broadcasts a Condition; `join` waits on it. That is exactly your team's A1 code.

</details>
