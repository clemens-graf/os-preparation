# A1-19 · waitpid

**Time box 40 min · ★★★ · type S (syscall + kernel, processes)**

```
Name:         waitpid (POSIX, wait.h - the stub exists)
Nr:           1717 (sc_waitpid)
Description:  waitpid(pid, &status, options) waits until the child process pid of the caller
              has ended and stores its exit code in *status (if status is not NULL).
              A process killed by the kernel reports the exit code it was killed with
              (9999 for a page fault). A child can be waited for only once.
              options are ignored.
Parameters:   pid_t pid, int* status, int options
Return Value: pid, or -1 (not a child of the caller, unknown, already waited for)
```

## Test

`task start a1-19` → `task test` (`waitpid_basic.sweb` – uses your team's `fork` – then
`exit` for the leak check). Done when 7 checks pass and no `leaking` warning appears.

## Your team's code

- The libc stub: `userspace/libc/src/wait.c` (`wait.h`).
- Processes: `UserProcess` has `getPID()`, `getPPID()` (`fork` passes the parent's pid). It is
  deleted in the CleanupThread when its last thread is gone (`~UserProcess`), long after
  `Syscall::exit(code)` ran – the exit code has to be remembered somewhere.
- The `ProcessRegistry` (a kernel thread + singleton, `ProcessRegistry::instance()`) knows every
  process start/end (`processStart`, `processExit`, a Mutex + Condition): the natural place for
  "which pid is whose child, and how did it end".

## Hints

<details><summary>1 – where does the exit code survive the process?</summary>

Not in the `UserProcess` – it is deleted. A table in the `ProcessRegistry`: pid, ppid, finished,
exit code. Fill it when a process is created (constructor), mark it finished in `~UserProcess`.
`Syscall::exit` stores the code in the process first (`exit_code_`). Use a fixed array: memory
allocated while the system runs and still allocated at shutdown is reported as a leak.
</details>

<details><summary>2 – waiting</summary>

Mutex + Condition in the registry: `while (child not finished) wait();`, broadcast in
`processFinished`. Remove the record when it was waited for (second waitpid → -1).
</details>

<details><summary>3 – records nobody will ever ask for</summary>

When a process ends, the records of **its** children can go (nobody can wait for them any
more); a process started by the kernel (ppid 0) needs no record after it ended.
</details>

## Questions a tutor might ask

- Why can the exit code not stay in the `UserProcess`? When exactly is that object deleted?
- What is a zombie? Where are they in your solution, and when do they disappear?
- The parent ends before the child – what happens to the child's record?
- Why write `*status` only after the registry lock is released?
