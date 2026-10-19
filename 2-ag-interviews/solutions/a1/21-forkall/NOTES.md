# A1-21 forkall – solution notes

Patch: [`solution.patch`](solution.patch) – on your team's code (`f78faff3`). Type S.

## The plan

```cpp
size_t UserProcess::forkall(UserThread* caller)
{
  Loader* child_loader = loadExecutable(...);            // disk i/o: before the lock
  {
    ScopeLock lock(thread_lock_);                        // list, slots, page tables frozen
    child = new UserProcess(..., new ArchMemory(*arch_memory_));   // deep copy: all stacks too
    child->next_stack_slot_ = next_stack_slot_;
    for (each UserThreadData* data in threads_)
    {
      UserThreadData* data_copy = new UserThreadData(*data);   // also for ended, unjoined threads
      data_copy->joined_ = false;
      if (data->thread_)                                  // still alive: copy the thread
      {
        UserThread* copy = new UserThread(child, data->thread_);   // registers, rax = 0
        copy->setTID(data->tid_);                         // same pthread_t in the child
        copy->setUserStackTop(...); copy->setUserStackLowestMappedVpn(...);
        if (data->thread_ != caller)
          copy->user_registers_->rax = data->thread_->in_syscall_ ? -1 : original rax;
        copy->setRegisteredInProcess(true);
        child->running_threads_++;
        data_copy->thread_ = copy;
      }
      child->threads_.push_back(data_copy);
    }
  }
  ScopeLock child_lock(child->thread_lock_);              // nobody in the child may exit yet
  child->startProcess(caller_copy);                       // ProcessRegistry + scheduler
  for (other copies) Scheduler::instance()->addNewThread(copy);
  return child->getPID();
}
```

`in_syscall_` (new, in `UserThread`) is set at the top of `Syscall::syscallException` and cleared
before it returns.

## Why each step

- **Where the state of a thread is:** a thread that is not running has its user registers saved
  in `user_registers_` – at the last timer interrupt (it was in user mode) or at `int 0x80` (it is
  in a syscall). Its stack is part of the address space, which the deep copy duplicates. Registers
  + stack = the thread. That is why the copies can simply continue.
- **A thread in a syscall:** its kernel half (kernel stack, the Condition it sleeps on, a held
  lock) belongs to the parent and is not copied. The copy in the child continues in user mode
  right after `int 0x80` – so it needs a return value: -1, "the call failed". Its parent original
  is untouched (the test checks that the parent's joiner still waits).
- **Page fault vs. syscall:** a page fault also enters the kernel (`switch_to_userspace_ == 0`),
  but its saved `rip` points AT the faulting instruction, which runs again – its `rax` must stay.
  Hence the separate `in_syscall_` flag.
- **Same tids:** `pthread_t` values are stored in user memory (the test's `spinner_tid`). With new
  tids the child could not join its own threads. Tids only have to be unique inside a process
  (`findThreadDataByTid` searches one process).
- **Records of ended threads** are copied too: in the parent they could still be joined – in the
  child as well.
- **The child's lock while scheduling:** the caller's copy could run at once and call `exit()`;
  `exitProcess` kills every thread in the list – a thread that is not in the scheduler yet would
  never be cleaned up. With the child's `thread_lock_` held, `exitProcess` waits until all are known.
- **Lock order** parent → child cannot deadlock: nobody else knows the child yet.

## The fork bug you find on the way

Your team's `UserThread(child_process, parent_thread)` does not copy `user_stack_top_` and
`lowest_mapped_stack_vpn_` (they stay 0). `isUserStackFault` then never matches in the child: a
forked child that needs a stack page the parent never touched dies with **666** (checked on
`f78faff3`: a child with ~12 KiB of locals). forkall copies both; for `fork` the same two lines
belong into the copy constructor.

## What is not perfect (say it before the tutor does)

- **Not an atomic snapshot:** the other threads of the parent keep running while the address
  space is copied (they write memory without any lock). A thread may be copied with registers
  from one moment and stack contents from a slightly later one. A real fix stops the other
  threads first (e.g. a "please stop" flag checked at every syscall and timer tick, wait until all
  are stopped) – much more code.
- `loadExecutable` opens the binary again for the child – as your team's `fork` does.
