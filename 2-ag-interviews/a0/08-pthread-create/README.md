# A0-08 · pthread_create – the mechanics

**Time box 60 min · ★★★ · read first: [ex06](../../examples/ex06-kernel-thread/), [ex02](../../examples/ex02-map-page/)**

```
Name:         pthread_create, pthread_exit (minimal)
Nr:           1550 (sc_pthread_create), 1551 (sc_pthread_exit)
Description:  pthread_create starts a new thread in the calling process that runs
              start_routine(arg) on its own user stack, in the same address space.
              When start_routine returns, the thread ends (libc calls pthread_exit).
              pthread_exit ends the calling thread; called by the main thread it behaves
              like exit().
              The process (its address space) must stay alive until all its threads are
              gone: when main returns while threads still run, the process waits for them.
Parameters:   pthread_t* thread (receives an id > 0, unique in the process),
              const pthread_attr_t* attr (ignored), void* (*start_routine)(void*), void* arg
Return Value: 0, or -1 (bad pointers, no more threads possible)
Notes:        Simplifications for A0: no pthread_join, exit() from a thread other than
              main ends only that thread, at most 64 threads per process lifetime.
              Each thread gets a stack of 4 pages.
```

## Test

`task start a0-08`, implement, `task test` (runs `pthread_create_basic.sweb`, `pthread_create_exit.sweb`, then `exit`).
Done when 9 checks pass, *"thread finished after main returned"* appears, and after
`exit` there is no `leaking` warning (all stacks and page tables freed).

## Hints

<details><summary>1 – what IS a thread in SWEB?</summary>

A `Thread` object = a kernel stack + kernel registers + (for user threads) user registers
+ a `loader_` (address space) + an entry in the scheduler's list. In base SWEB,
`UserProcess` *is* the only thread of a process. Look at its constructor: it maps one stack
page, creates user registers (`ArchThreads::createUserRegisters`), sets the address space
(`ArchThreads::setAddressSpace`) and `ProcessRegistry` adds it to the scheduler. A new
thread needs the same steps – with the **same** `loader_` and a **different** stack.
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

`UserProcess::~UserProcess` deletes `loader_` (and with it the page tables). If the main
thread dies first, the other threads run on freed page tables. Count the threads of the
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

- Which registers did you set for the new thread and why exactly those?
- Why does the thread not start at `start_routine` directly?
- What is shared between the threads, what is private?
- When exactly is the stack of a finished thread unmapped – and why not in `pthread_exit`?
- What happens if `thread` points to an unmapped page? (order of operations in your syscall!)
- What changes for `pthread_join` (A1)?
