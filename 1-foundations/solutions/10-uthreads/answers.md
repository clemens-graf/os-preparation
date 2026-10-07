# Module 10 — Answers

**Q1.** A thread must end by *exiting*: becoming a zombie with its return
value, waking the joiner, being scheduled away. A plain `return` from the
start routine does none of that. With `uc_link = NULL`, returning from
the context's function ends the whole process (glibc calls `exit`), and
the return value is lost. The trampoline turns
`return v` into `uthread_exit(v)`. In SWEB the new user thread's first
instruction is wherever the kernel sets `rip`. If that is `start_routine`
itself, its `ret` pops a garbage return address from the fresh user stack
and jumps into nowhere (page fault, process killed). Fix: libc's
`pthread_create` passes the kernel a *wrapper* (for example
`void wrapper(fn, arg) { pthread_exit(fn(arg)); }`) as the entry point,
and `fn`/`arg` as its arguments in registers. Returning then calls
`pthread_exit`. Alternatively the kernel places the address of such a
wrapper as the return address on the new stack.

**Q2.** `uthread_exit` runs *on* that stack: unmapping it pulls the
memory out from under the running code, which crashes at the next push or
return (the `free_own_stack` mutant segfaults). The library records the
thread in `reap_me`, and the next thread to run frees it right after the
switch (`after_switch`, including in `trampoline`). SWEB: `Thread::kill`
only sets the state to `ToBeDestroyed` and yields. The scheduler never
picks such a thread again, and the separate **cleanup thread**
(`Scheduler::cleanupDeadThreads`) later `delete`s it, running on its own
stack.

**Q3.** A thread that has ended but not been joined yet. It keeps its id,
its return value and its slot. Its stack could go as soon as it has
switched away (this library keeps it until join/detach for simplicity).
A thread that is never joined or detached stays a zombie forever: a leak
that eventually runs into `EAGAIN`. The analogue is a zombie *process*,
which `wait()` reaps (module 01).

**Q4.** Invariant: the `joining` pointers form chains without cycles,
because every join that would close a cycle is refused. Starting from the
target and following `joining` therefore ends after at most `UTHREAD_MAX`
steps, at a thread that is not joining anyone. The new edge
"current → target" closes a cycle exactly when the chain from target
leads back to current, which is what the walk checks. POSIX says "may"
because detection costs work and bookkeeping (and for mutexes the general
wait-for graph). Implementations are only required to report what they
detect cheaply; glibc detects self-joins, for example.

**Q5.** Hand-off: strict FIFO fairness, no starvation, and a waiter that
is woken is sure to own the mutex. But every unlock with waiters forces
a switch-order dependency: the unlocker cannot re-take the lock even
while the woken thread has not run yet. Every lock becomes a context
switch, which forms "lock convoys". Barging: the running unlocker (or
anybody) can grab the free mutex again immediately, giving higher
throughput and fewer switches, but a waiter can be overtaken again and
again (unfair, possible starvation), and woken waiters must loop. SWEB's
`Mutex::acquire` **barges**: it loops on `testSetLock`, and a woken
thread tries again and may go back to sleep.

**Q6.** Without preemption nothing else runs between "unlock" and
"block", because a switch happens only at `schedule()`, so no signal can
be lost in between. With preemption, a tick after the unlock lets another
thread take the mutex, change the condition and `signal`. That happens
before we are on the waiters list (lost), or while our state is half set.
Part C runs the whole sequence with SIGVTALRM blocked: interrupts off.
Waiters list, unlock and state change then happen at one instant. SWEB:
the waiter is pushed onto the waiters list *before* the list lock is
released, so a signaller will find it. `Scheduler::wake` yields in a loop
until the thread really is `Sleeping`. Even if the signal comes between
`unlockWaitersList()` and `Scheduler::sleep()`, the waker waits until the
sleep has happened and then sets the thread back to `Running`. The
wake-up cannot be lost.

**Q7.** Mutex: thread A reads `owner == -1`, and the tick hits before
`owner = A`. Thread B also sees `-1` and takes the mutex. A resumes and
overwrites `owner = A`: two owners, and mutual exclusion is broken. Queue:
`item[head + len] = id` is written, the tick hits, another thread pushes
into the *same* index (len unchanged) and increments `len`, then A
increments `len` again. One id is lost and one slot holds garbage: a
thread that is never scheduled again, or a crash. Blocking the signal
corresponds to `ArchInterrupts::disableInterrupts()` (or the lighter
`Scheduler::lockScheduling()`) in SWEB. On a multi-core machine the
other cores keep running, so disabling interrupts locally does not stop
them from entering the same code. You additionally need spinlocks (or
other atomic operations) around shared kernel data.

**Q8.** The tick can interrupt a thread in the middle of `printf` or
`malloc`, which then hold an internal lock. Another uthread calling them
runs on the *same* kernel thread and blocks on that lock (or corrupts the
structure, if the lock is recursive), and the owner cannot run to release
it. The functions are not *async-signal-safe*. SWEB: no `new`/`delete` in
interrupt handlers or with interrupts off (the kernel heap lock), which
is why `Scheduler::schedule` and `Thread::kill` say so in comments.

**Q9.** M:1: a switch is a function call plus register save/restore (the
measured ~0.4 µs is mostly glibc's signal-mask system call; without it,
tens of ns). No kernel involvement. But a blocking system call
(`read()` on the keyboard) blocks the one kernel thread and with it
*every* uthread, and all uthreads share one core. 1:1: each switch goes
through the kernel (µs), but a blocked thread blocks only itself, and
threads run in parallel on several cores. A SWEB thread has its own
**kernel stack** (for system calls and interrupts), its own **user
stack**, two register sets (`kernel_registers_`/`user_registers_`), a
TID, a state known to the scheduler, and entries in lock lists. It shares
the process's address space (`Loader`/`ArchMemory`) and files.

**Q10.** (one possible design)
- **System calls:** new numbers in `syscall-definitions.h`, cases in
  `Syscall::syscallException`, wrappers in `userspace/libc/src/pthread.c`.
  Check every user pointer (`thread`, `retval` for join) against
  `USER_BREAK` with overflow-safe checks. Never dereference `arg`.
- **Create:** a new `UserThread` (or `Thread` subclass) that shares the
  process's `Loader`/`ArchMemory`, gets its own kernel stack (part of
  `Thread`) and a **user stack** in a free slot below the existing
  stacks, with a guard gap. Mapped on demand (module 08, Q9), or a few
  pages eagerly. `rip` = the libc wrapper, arguments = `start_routine`
  and `arg` in registers, `rsp` = top of the new stack. Register it with
  the process's thread list and `Scheduler::addNewThread`.
- **Per thread for join:** TID, state (running / zombie), return value,
  detached flag, and who is joining it. Keep this record **after** the
  `Thread` object is destroyed (or delay destruction until joined), since
  a join may come after the thread has long gone.
- **Exit:** store the return value, mark as finished, wake a joiner
  (`Condition::signal`/`broadcast` under the process lock). A detached
  thread frees its record, and the last thread of the process ends the
  process. The stack is freed by the cleanup thread, not by the thread
  itself (Q2). The user stack pages can be unmapped at exit or at join.
- **Join:** under the process lock: check ESRCH/EDEADLK/EINVAL (self,
  cycle, detached, already joined). While the target has not finished:
  `Condition::wait` (no busy loop, module 09 Q9). Then copy the return
  value to user space (pointer checked) and free the record.
- **Detach:** mark detached, or free at once if already finished.
- **Races:** threads of one process can now fault and make system calls
  concurrently, so address-space operations need a lock (module 08,
  Q10).
