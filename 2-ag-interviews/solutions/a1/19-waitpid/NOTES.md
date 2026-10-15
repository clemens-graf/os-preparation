# A1-19 waitpid – solution notes

Patch: [`solution.patch`](solution.patch). Type S.

| Piece | Where |
|---|---|
| `ProcessRecord records_[64]` (used, finished, pid, ppid, exit_code), `records_lock_`, `process_finished_` (Condition) | `ProcessRegistry.h` |
| `processCreated(pid, ppid)`, `processFinished(pid, code)`, `waitForChild(ppid, pid, &code)` | `ProcessRegistry.cpp` |
| `exit_code_` (public, = 0) | `UserProcess.h` |
| `processCreated` in the constructor, `processFinished` in `~UserProcess` | `UserProcess.cpp` |
| `process->exit_code_ = exit_code;` before `exitProcess()` | `Syscall::exit` |
| `Syscall::waitpid`, `case sc_waitpid` (1717) | `Syscall.cpp` |
| `waitpid` → `__syscall` | `userspace/libc/src/wait.c` |

```cpp
size_t ProcessRegistry::waitForChild(size_t ppid, size_t pid, size_t* exit_code)
{
  ScopeLock lock(records_lock_);
  while (true)
  {
    ProcessRecord* child = /* used && pid == pid && ppid == ppid */;
    if (!child) return -1;                       // not ours / unknown / reaped
    if (child->finished) { *exit_code = child->exit_code; child->used = false; return 0; }
    process_finished_.wait();                    // broadcast in processFinished
  }
}
```

- **Why the registry:** `~UserProcess` runs in the CleanupThread some time after the last thread
  died; the parent may call `waitpid` much later. The exit code needs a place that outlives the
  process – the same idea as `UserThreadData` for `pthread_join`.
- **Zombies:** a finished record nobody waited for yet. They go away when the parent waits, or
  when the parent itself ends (`processFinished` drops the records of its children), or at once
  for processes started by the kernel (ppid 0: the shell). The shell's own children (every test
  program) have the shell as parent: their records go when the shell ends.
- **Fixed array, not a list:** SWEB traces the kernel heap while the programs run and reports
  everything still allocated at shutdown as a leak – a growing `ustl::list` of records would.
- A killed process (page fault → `Syscall::exit(9999)`) goes through the same `Syscall::exit`, so
  its code is 9999.
- `*status` is written after `waitForChild` returned: writing user memory can page-fault, never
  do that holding a lock.
