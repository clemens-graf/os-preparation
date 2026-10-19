# A0-08 pthread_create – solution notes

## In your team's code: read and explain

Your team's `main` already has `pthread_create` (Michael), so the team version of this task is a
**study task**: `task start a0-08` / `task solution a0-08` give you `main` plus the course tests,
and `task test` runs them against your team's implementation – all 9 pass. Returning from `main`
is `exit()` and ends the whole process (`UserProcess::exitProcess` kills all threads, as POSIX
says); `pthread_exit` in `main` ends only the main thread – the process lives on until its last
thread has exited (the last `UserThread`'s destructor deletes the process).

Where things are in your team's code:

| Step | Your team's code |
|---|---|
| syscall, checks | `Syscall::pthreadCreate` (`Syscall.cpp`) |
| stack slot + first stack page | `UserProcess::allocateNewStack` → `allocateNewStackSlotNumber`, `mapStackPage`; slots of `USERSTACK_PAGES + GUARD_PAGE` pages |
| the thread object | `UserProcess::createThread` (under `thread_lock_`) → `new UserThread(process, …, entry, true, start_routine, arg, stack_top)` |
| registers | `UserThread` constructor: `createUserRegisters(rip = entry, rsp = stack_top)`, `setThreadArguments` (rdi/rsi), `setAddressSpace` |
| bookkeeping | `registerThread` → a `UserThreadData` in `threads_` (tid, exit value, exited/joined – your join uses it) |
| start | `Scheduler::addNewThread` in the syscall, after `*thread = tid` |
| stack growth | page fault handler: `isUserStackFault` → map pages down to the fault; guard page → kill |
| end | `exitThread` (stack pages freed, value stored, joiners woken) / `exitProcess` (kills every thread) |

Locking in your team's code: one Mutex per process, `thread_lock_`, for the thread list, the stack
slots and the page tables (the page fault handler takes it for stack pages).

## Answers to the tutor questions

- *Registers:* `rip` (entry), `rsp` (own stack), `rdi`/`rsi` (arguments), segment registers
  and `rflags` (by `createUserRegisters`: user segments, interrupts on), `cr3` (by
  `setAddressSpace`), `rsp0` (kernel stack for the next interrupt).
- *Shared:* the `UserProcess`: address space (code, data, heap), loader, files, working dir. *Private:* user
  stack, registers, kernel stack.
