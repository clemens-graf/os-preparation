# Module 11 — Answers

Checked against `osw26e3` as of 2026-09-24 (upstream state, commit
`f6fcb2ab`). Line numbers drift; function names do not.

## Part 0

1. `mult.c`'s `main` returns the sum. `_start` (`userspace/libc/src/nonstd.c`)
   calls `exit(main())`, the result travels in `rbx` as the argument of
   `sc_exit`, and `Syscall::exit` prints it:
   `[SYSCALL] Syscall::EXIT: called, exit_code: 1237619379` — the value in
   `mult.c`'s comment. (The same number also appears once more in the
   `Syscall 1 called with arguments ...` line.)
2. `common/include/console/debug.h`: every category is a colour ORed with
   `OUTPUT_ENABLED`. Remove the flag from `SYSCALL`:
   `const size_t SYSCALL = Ansi_Blue;`.
3. `userspace/tests/CMakeLists.txt` collects `*.c` with `file(GLOB ...)` at
   **configure** time: re-run cmake (*Reload CMake Project*), then build.
   Every `.c` there becomes `<name>.sweb` on the minix partition (mounted at
   `/usr`), typed in the shell as `hello.sweb`. The "shell.sweb failed"
   error after adding a program is a parallel-build race; a second build
   (or `make` without `-j`) fixes it.
4. `make qemugdb` starts QEMU with `-s -S` (gdb server on port 1234, CPU
   halted). In a second terminal `make rungdb` starts gdb with
   `utils/gdbinit`, which does `target remote 127.0.0.1:1234`. Then
   `break Syscall::write`, `continue`.

## Part 1

| What | File(s) |
|---|---|
| kernel entry after boot | `common/source/kernel/main.cpp` (`startup()`) |
| system call dispatcher | `common/source/kernel/Syscall.cpp` (`Syscall::syscallException`) |
| user-side `__syscall` | `arch/x86/64/userspace/syscalls.c` |
| syscall numbers | `common/include/kernel/syscall-definitions.h` |
| libc `pthread_*` stubs | `userspace/libc/src/pthread.c`, `userspace/libc/include/pthread.h` |
| `Thread` | `common/include/kernel/Thread.h`, `common/source/kernel/Thread.cpp`; the kernel stack is `uint32 kernel_stack_[2048]` — 8 KiB *inside the Thread object* |
| `UserProcess` | `common/include/kernel/UserProcess.h`: `class UserProcess : public Thread` |
| scheduler | `common/source/kernel/Scheduler.cpp` (`threads_`) |
| deletes dead threads | `common/source/kernel/CleanupThread.cpp` → `Scheduler::cleanupDeadThreads` |
| page tables | `arch/x86/64/source/ArchMemory.cpp` (`mapPage`, `unmapPage`, `resolveMapping`) |
| page faults | `common/source/mm/PageFaultHandler.cpp`, `common/source/kernel/Loader.cpp` (`loadPage`) |
| frame allocator | `common/source/mm/PageManager.cpp` (`allocPPN`, `freePPN`) |
| locks | `common/source/kernel/{Lock,Mutex,Condition,SpinLock}.cpp` |
| `USER_BREAK`, identity map | `arch/x86/64/include/offsets.h` (`USER_BREAK`, `IDENT_MAPPING_START`) |
| first user program | `common/include/kernel/user_progs.h` → `/usr/shell.sweb`, started by `ProcessRegistry::Run` |

## Part 2

**T1.** Before the shell: `CleanupThread` and `IdleThread` (created by the
`Scheduler` constructor), `KprintfFlushingThread`, the console
(`TxTConsoleThread`) and `ProcessRegistry` (the debug output prints this
list). `UserProcess` opens the binary, creates a `Loader` that reads
only the ELF headers (`loadExecutableAndInitProcess`), allocates **one**
frame and maps it at virtual page `USER_BREAK/PAGE_SIZE − 1`
(`0x7ffffffff000`). `rsp = USER_BREAK − 8`. Code and data are *not*
loaded: every first access page-faults (`[LOADER] loadPage` lines). The
first instruction is the ELF entry `_start` (`nonstd.c`), which calls
`exit(main())`, so returning from `main` exits.

**T2.** `write()` → `__syscall(sc_write, fd, buf, count, 0, 0)` →
`int $0x80` → `arch_syscallHandler` (`pushAll`,
`arch_saveThreadRegisters` → `user_registers_`) → `syscallHandler`
(interrupts on) → `Syscall::syscallException(rax, rbx, rcx, rdx, rsi,
rdi)` → `Syscall::write` → `kprintf`. Back: `user_registers_->rax = ret`,
`arch_contextSwitch`, `iretq`. Number in `rax`, arguments in `rbx, rcx,
rdx, rsi, rdi`: **five arguments, no register left** for a sixth.
`pthread_create` with a wrapper needs exactly five (thread, attr, start,
arg, wrapper). The check `buffer + size > USER_BREAK` can overflow
(module 07, Q2).

**T3.** Round robin, one timer tick per quantum. `schedulable()` means
`state == Running`: `Sleeping` threads are skipped. The idle thread is
always runnable. `yield` → `int $65` → `irqHandler_65` → `schedule()`:
the same switch, but at the thread's request (no tick counted, no
preemption). `wake` loops until the target really is `Sleeping`, so the
wake-up cannot get lost (module 09, Q8).

**T4.** Accepted: non-present user addresses (≥ `PAGE_SIZE`,
< `USER_BREAK`), from user or kernel mode (a syscall touching an unloaded
user page is fine). Rejected: the null page, kernel addresses, and faults
on present pages (protection). The process is ended with
`Syscall::exit(9999)`. Below the stack page: `checkPageFaultIsValid`
accepts it, but `Loader::loadPage` finds no ELF segment for the address
("No section refers to the given address") and calls `Syscall::exit(666)`.
The stack cannot grow. Shared between threads of a process: the `Loader`,
its `ArchMemory` (page tables), the binary's fd. `program_binary_lock_`
protects only the reads from the binary. `mapPage`/`unmapPage` have **no
lock**: two faults can both allocate the same missing page table
(module 08, Q10). `PageManager` has its own lock.

**T5.** `Syscall::exit` → `currentThread->kill()`: `state = ToBeDestroyed`,
then `yield`, never to be scheduled again. `CleanupThread::Run` →
`Scheduler::cleanupDeadThreads` removes such threads from `threads_` and
`delete`s them. Deferred because (a) the thread's kernel stack is part of
the object it would delete, and it is running on it, and (b) `kill` may
be called with interrupts off, where `delete` (kernel heap lock) is
forbidden. `~UserProcess` deletes the `Loader`, which deletes the
`ArchMemory`: every page table and user frame. It also closes the binary,
deletes the working directory and calls `ProcessRegistry::processExit`.
A second thread of the process still running would lose its address
space underneath it: crash, or worse, frames reused elsewhere. In the
log: `Syscall::EXIT` → `kill: Called by ...` → `~Thread: freeing
ThreadInfos` → `cleanupDeadThreads: done`.

**T6.** `Mutex::acquire` sleeps: `testSetLock`; if taken, lock the
waiters list, try once more, then `sleepAndRelease`. Woken up, it loops
and tries again: **barging**, since another thread may take the mutex
first. `Condition::wait`: the waiter is put on the waiters list *while
the list lock is held*. So a signaller (which needs that lock) either
comes before (the waiter is not waiting yet, and holds the mutex, so the
signaller cannot have changed the condition) or finds it on the list.
Between unlocking the list and `Scheduler::sleep()`, a signal can arrive
and call `Scheduler::wake`, which yields until the waiter really sleeps
and then sets it `Running`. No lost wake-up. The lists enable deadlock
and misuse checks (`doChecksBeforeWaiting`): a thread waiting for a lock
held by a thread that waits for one it holds, recursive locking,
releasing a lock one does not hold, dying with locks held (`~Thread`
asserts). `Condition::wait` asserts that no other lock is held while
sleeping, because sleeping with an unrelated lock held blocks everyone
who needs it and is the classic path to deadlock.

## Part 3 — a reference split (one of several good ones)

| Member | Owner | Why |
|---|---|---|
| `kernel_stack_`, `kernel_registers_`, `user_registers_`, `switch_to_userspace_` | thread | per flow of execution |
| `state_`, `tid_`, lock lists (`next_thread_in_lock_waiters_list_`, `lock_waiting_on_`, `holding_lock_list_`) | thread | the scheduler and locks see threads |
| `loader_` (address space), binary `fd_` | **process** | shared by all threads |
| `working_dir_`, terminal | process | POSIX: per process (all threads print to the same terminal) |
| file descriptor table | process | P2 elective: per-process fds |
| name | both | process name = binary; thread names for debugging |
| **new:** PID, list/count of threads, a process lock, exit status, stack-slot map, join records | process | |

- **Ids.** Every thread prints `0`: `tid_` is initialised to 0 and never
  set. Hand out TIDs (and PIDs) from a global counter under a lock, or
  with an atomic increment (`ArchThreads::atomic_add`). Do not reuse an id
  while a join record for it can still exist.
- **Finding the process.** 26 uses of `loader_` in 11 files (several in
  32-bit/ARM code). The least invasive split: `UserThread` gets a
  `UserProcess* process_`, and `Thread::loader_` stays as a *non-owning*
  shortcut that the process sets for each of its threads. Most uses keep
  working unchanged; only ownership and deletion move.
- **Lifetime.** The process lives until its last thread is destroyed.
  Count threads under the process lock. When `cleanupDeadThreads` deletes
  the last `UserThread` of a process, it also deletes the process (its
  `Loader`/`ArchMemory`). That runs in the cleanup thread, on another
  stack and another CR3, which matters because `~ArchMemory` asserts
  that a thread never deletes the address space it runs in.
- **Counting.** `ProcessRegistry::processStart/processExit` once per
  *process*: at process creation and at process destruction, not per
  thread.
- **Exit.** Today it kills the one thread, which is the process. After
  the split it must end all threads of the process, each at a safe point
  (module 07, Q7). For Phase 0 (one thread per process) the behaviour
  stays the same. The full fix is the "`exit()` syscall fix" of the A1
  plan: agree who owns it.
- **Files touched:** `UserProcess.h/.cpp`, a new `UserThread.h/.cpp`,
  `Thread.h/.cpp` (ids), `ProcessRegistry.cpp`, `Scheduler.cpp`
  (cleanup), `Syscall.cpp` (exit), and wherever `loader_` ownership
  matters.

## Part 4 — P1 sketch

1. The libc `pthread_create(thread, attr, start, arg)` calls
   `__syscall(sc_pthread_create, thread, attr, start, arg, wrapper)` with
   `wrapper(start, arg) { pthread_exit(start(arg)); }` as the real entry
   point.
2. `size_t Syscall::pthreadCreate(size_t thread, size_t attr, size_t start,
   size_t arg, size_t wrapper)`: `thread` must lie in user space and hold a
   `pthread_t` (overflow-safe check, *before* creating anything). `start`
   and `wrapper` must be non-NULL and `< USER_BREAK`. `attr` is NULL or
   ignored/checked. `arg` is **not checked**. Errors: POSIX error numbers
   (`EINVAL`, `EAGAIN`) returned as the syscall result.
3. `ArchThreads::createUserRegisters(user_registers_, (void*)wrapper,
   (void*)stack_top, getKernelStackStartPointer())`, then
   `user_registers_->rdi = start`, `user_registers_->rsi = arg` (System V
   ABI: first two arguments). `ArchThreads::setAddressSpace(thread,
   process->loader_->arch_memory_)` sets `cr3`. `switch_to_userspace_ = 1`,
   then `Scheduler::addNewThread`. Keep the ABI's stack alignment: like
   `UserProcess`, start at the slot top minus 8, as if a `call` had pushed
   a return address.
4. Stack slot *i* below the main stack, with an unmapped guard page per
   slot. Map the top page eagerly (as `UserProcess` does), and teach the
   page-fault path to allocate zero pages inside a known stack slot
   instead of asking the ELF loader. Record free/used slots in the process
   and unmap/free the slot at join (or at exit, for detached threads).
5. Keep a join record per TID in the process: finished flag, return
   value, detached flag, joiner. The `Thread` object itself is deleted by
   the cleanup thread long before a late join. The joiner waits on a
   `Condition` bound to the process lock: `while (!rec->finished)
   cond.wait()`. Exit sets the flag and signals, under that lock. Errors:
   ESRCH, EDEADLK (self or cycle), EINVAL (detached, or already being
   joined). The return value is copied to the user's `value_ptr` after a
   pointer check.
6. Test ideas: one thread writes a global, main yields until it sees it;
   argument passing (pointer to a struct); 50 threads each incrementing
   their own slot; a thread creating threads; the start routine returns
   (no crash, process alive); main exits while threads still run; a
   non-user `pthread_t*` → error, nothing created; join returns the exact
   value; join self → error; join twice → error; join a detached thread →
   error; 100 create/join cycles (no leak: stack slots reused). Each one
   ten times with `sweb_run.py -n 10`.
