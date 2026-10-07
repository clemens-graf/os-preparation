# A0-08 pthread_create – solution notes

Patch: [`solution.patch`](solution.patch). This is the biggest A0 task – and the mechanics
P1 implements in the team repo for A1. Know every step.

## Files touched

| File | What |
|---|---|
| `Thread.h/.cpp` | `UserProcess* process_` (nullptr for kernel threads) |
| `UserProcess.h/.cpp` | `process_ = this`; `threads_lock_`, `threads_done_`, `num_threads_`, `next_stack_slot_`; `createThread()`, `threadFinished()`, `waitForThreads()` |
| **new** `UserThread.h/.cpp` | the additional thread: stack, registers, address space; destructor unmaps the stack |
| `Loader.h/.cpp` | page-table lock, now really needed: two threads share one `ArchMemory` |
| `Syscall.h/.cpp` | `pthreadCreate`, `pthreadExit`; `exit()` of the main thread waits for the others |
| `syscall-definitions.h` | 1550, 1551 |
| `userspace/libc/src/pthread.c` | `pthreadStart()`, `pthread_create()`, `pthread_exit()` |

New `.cpp` files are found by cmake's glob – re-run cmake (`run_test.sh` always does).

## A thread = 4 things

```cpp
UserThread::UserThread(UserProcess* process, size_t stack_slot, size_t entry,
                       size_t start_routine, size_t argument) :
    Thread(process->getWorkingDirInfo(), process->getName(), Thread::USER_THREAD), stack_slot_(stack_slot)
{
  process_ = process;
  loader_ = process->loader_;                          // 1. the SAME address space

  for (size_t i = 1; i <= USER_THREAD_STACK_PAGES; ++i) // 2. its OWN stack
  {
    size_t ppn = PageManager::instance()->allocPPN();
    loader_->arch_memory_lock_.acquire();
    bool mapped = loader_->arch_memory_.mapPage(stackTop() / PAGE_SIZE - i, ppn, 1);
    loader_->arch_memory_lock_.release();
    assert(mapped);
  }
  ArchThreads::createUserRegisters(user_registers_, (void*) entry,     // 3. its OWN registers
                                   (void*) (stackTop() - sizeof(pointer)), getKernelStackStartPointer());
  user_registers_->rdi = start_routine;
  user_registers_->rsi = argument;
  ArchThreads::setAddressSpace(this, loader_->arch_memory_);          //    cr3 = the process's PML4
  setTerminal(process->getTerminal());
}
// 4. the scheduler: Scheduler::instance()->addNewThread(thread) in the syscall
```

- The `Thread` constructor already created the kernel stack and kernel registers, and set
  `switch_to_userspace_ = 1` for `USER_THREAD`. The first time the scheduler picks the
  thread, `arch_contextSwitch` loads the **user** registers → it starts in user mode at `entry`.
- `entry` = libc's `pthreadStart(start_routine, arg)`: `pthread_exit(start_routine(arg))`.
  That is why returning from `start_routine` ends the thread.
- `rsp = stackTop - 8`: at function entry the ABI expects `rsp % 16 == 8` (as if `call`
  had pushed a return address) – the main thread uses `USER_BREAK - 8` for the same reason.

## The syscall – order matters

```cpp
size_t Syscall::pthreadCreate(size_t thread_id, size_t start_routine, size_t argument, size_t entry)
{
  if (/* bad pointers */) return (size_t) -1;
  *(size_t*) thread_id = 0;          // 1. touch it: if this faults, nothing exists yet
  UserThread* thread = currentThread->process_->createThread(entry, start_routine, argument);
  if (!thread) return (size_t) -1;
  *(size_t*) thread_id = thread->getThreadId();   // 2. id valid before the thread runs
  Scheduler::instance()->addNewThread(thread);    // 3. from here on it may run any time
  return 0;
}
```

If the write to `thread_id` faulted *after* `createThread`, the process would be killed
with a counted but never started thread – and the main thread's `exit` would wait forever.

## Lifetime and locking

```cpp
UserThread* UserProcess::createThread(...)
{
  threads_lock_.acquire();
  if (next_stack_slot_ > MAX_USER_THREADS) { threads_lock_.release(); return nullptr; }
  size_t slot = next_stack_slot_++;
  ++num_threads_;
  threads_lock_.release();
  return new UserThread(this, slot, entry, start_routine, argument);   // outside the lock
}
void UserProcess::threadFinished()      // from ~UserThread, in the CleanupThread
{
  threads_lock_.acquire();
  if (--num_threads_ == 0)
    threads_done_.signal();
  threads_lock_.release();
}
void UserProcess::waitForThreads()      // from Syscall::exit of the main thread
{
  threads_lock_.acquire();
  while (num_threads_ > 0)
    threads_done_.wait();
  threads_lock_.release();
}
```

| Data | Lock |
|---|---|
| page tables (now shared by several threads) | `Loader::arch_memory_lock_` – around **every** `mapPage`/`unmapPage`, including `Loader::loadPage` |
| `num_threads_`, `next_stack_slot_` | `UserProcess::threads_lock_` + `Condition threads_done_` |

- **Why the page-table lock is now mandatory:** two threads page-fault at the same time,
  both run `mapPage` for addresses under the same not-yet-existing page table, both allocate
  a page table and write it into the same page-directory entry → one table (and the mapping
  in it) is lost, one page leaks.
- **No nested locks:** `createThread` releases `threads_lock_` before constructing the thread
  (which takes `arch_memory_lock_` and allocates).
- **`while`, not `if`, around `wait()`**; the state (`num_threads_`) changes under the same mutex
  as the signal.
- **The stack is unmapped in the destructor**, not in `pthread_exit`: a thread can end in
  several ways (`pthread_exit`, returning from `start_routine`, being killed by a page fault
  via `Syscall::exit(9999)`, later `pthread_cancel`). The destructor is the one place all of
  them pass, and it runs in the CleanupThread when the thread can never run again.
- `threadFinished()` is the **last** line of `~UserThread`: after it, the main thread may
  finish, and its destructor deletes `loader_` – which `~UserThread` used just before.

## Simplifications (and what A1 changes)

- `exit()` from a non-main thread ends only that thread (POSIX: ends the process).
- Stack slots are never reused; at most 64 threads per process lifetime.
- `pthread_exit`'s value is dropped – `pthread_join` (A1) needs it stored in the thread and
  the joiner woken up.

## Answers to the tutor questions

- *Registers:* `rip` (entry), `rsp` (own stack), `rdi`/`rsi` (arguments), segment registers
  and `rflags` (by `createUserRegisters`: user segments, interrupts on), `cr3` (by
  `setAddressSpace`), `rsp0` (kernel stack for the next interrupt).
- *Shared:* address space (code, data, heap), loader, files, working dir. *Private:* user
  stack, registers, kernel stack.
