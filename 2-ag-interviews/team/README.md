# Team mode – the A0 tasks on your team's code

In the interview you work in **your team repo**, not in base SWEB. Team mode lets you practise
exactly there: every A0 task and every example starts from a snapshot of your team's `main`
(`osw26e3`), and the reference solutions are written for your team's classes and names.

Everything in this folder was made and tested on the commit in [`BASE`](BASE)
(`osw26e3` main, 7 Oct 2026): all 11 A0 tasks and all 11 examples build and pass their tests.

## Use it

```bash
task team setup        # once: imports that commit from repos/osw26e3 as branch team/main (read only)
task mode team         # from now on: start/test/solution/diff/example/reset use team/main
task start a0-01       # -> branch team/task/a0-01-meminfo, on your team's code
task test
task solution a0-01    # -> branch team/solution/a0-01-meminfo
task mode base         # back to base SWEB; your team/... branches stay
```

- Branches of team mode live under `team/` (`team/main`, `team/task/...`, `team/solution/...`,
  `team/example/...`, `team/backup/...`), so base and team attempts never mix.
- `task list` and `task status` show the mode. Being on a `team/...` branch always means team mode.
- Your team repo is only read (`git fetch` from it into the training repo); nothing there changes.
- A1/A2 have no team version yet – in team mode `task start a1-01` says so.

## What is different in your team's code

| Base SWEB (task cards, base solutions) | Your team repo (team solutions) |
|---|---|
| `class UserProcess : public Thread` – the process *is* its only thread | `class UserProcess` (no thread) + `class UserThread : public Thread`, `UserProcess::createMainThread()` |
| `currentThread->loader_` | `currentThread->getLoader()` (`Thread::loader_` is gone, the loader belongs to the process) |
| `(UserProcess*) currentThread` | `currentThread->getProcess()` – the old cast still compiles, but reads garbage |
| per-process state in `Loader` (heap limit, segv handler) | per-process state in `UserProcess` |
| per-thread state in `Thread` (A0-11's note) | per-user-thread state in `UserThread`: `(UserThread*) currentThread` |
| stack page + user registers in the `UserProcess` constructor | in the `UserThread` constructor |
| syscall numbers as on the cards | 1500 is your `mkdir` → **meminfo uses 1505**; all other A0 numbers are free |
| `Loader::arch_memory_lock_` (added by the course) | your team has **no page-table lock** – the team solutions add the same one to the `Loader` |

Stays the same: `ArchMemory` (`mapPage`, `unmapPage`, `resolveMapping`, `checkAddressValid`),
`PageManager`, `Loader::loadPage`, `currentThread->user_registers_`, `Scheduler`, the six places of a
syscall.

## Per task

| Task | Team version |
|---|---|
| A0-01 meminfo | number 1505; `getLoader()->arch_memory_` + new `Loader::arch_memory_lock_` |
| A0-02 allocpage | `getLoader()` instead of `loader_` |
| A0-03 heaplimit | `heap_size_`, `mapHeapPage()`, `setHeapLimit()` in `UserProcess`; `HEAP_START` in `UserProcess.h` |
| A0-04 magic page | `getLoader()` in the page fault handler, `Loader::mapReadOnlyPage()` |
| A0-05 readonly code | unchanged (only `Loader::loadPage`) |
| A0-06 segv handler | `segv_entry_`, `segv_handler_`, `deliverSegv()` in `UserProcess` |
| A0-07 upcall | `getLoader()` for the stack check |
| A0-08 pthread_create | second `UserThread` constructor (stack slot), thread counting in `UserProcess`, only the main thread deletes the process |
| A0-09 pthread_multi | on top of A0-08; `UserThread::pushToStack()` |
| A0-11 thread note | `note_` in `UserThread` |
| A0-12 alias page | `getLoader()`, never the `(UserProcess*)` cast; alias list stays in the `Loader` |
| ex03 | comment: the stack page comes from the `UserThread` constructor |
| ex11 | `s_mutex` initialised after `name_` in your `Thread` constructor |
| ex12 | `getLoader()` instead of `(UserProcess*) currentThread` |
| other examples | only `getLoader()` and new places for the added lines |

The base cards (`../a0/NN-*/README.md`) each have a section **"In your team repo"** with the
details for that task.

## Found in your team's code – worth knowing before the interview

- **No lock for the page tables.** With one thread per process that is fine; with `pthread_create`
  two threads map pages concurrently. Expect the question.
- **`~UserThread` deletes the process** – for every thread. Correct only while each process has one
  thread; the A0-08 team solution shows the change.
- **`Syscall::mkdir` starts with `size_t result = -1U;`** – the same trap as tutorial 1: as a `size_t`
  that is `0xFFFFFFFF`, not -1. It only works because the `int` wrapper cuts it back to 32 bits.
  `(size_t) -1` is the safe spelling.
- **`Syscall::copyUserString()`** exists (byte by byte, stops at `USER_BREAK`) – reuse it where a string
  must fit completely; it refuses longer strings instead of cutting them (see A0-11).
- GRUB boots without waiting (`timeout 0` in `menu.lst`) – `task run` starts SWEB directly.

## When your team's main moves on

`team/main` stays at the commit in `BASE`, so the team solutions always apply. To practise on a newer
team state, the team patches have to be re-made on that commit (the base patches in `../solutions`
and `../examples` are the starting point) and `BASE` updated. Your own attempts on `team/task/...`
branches are not affected.

These patches contain lines of your team's code (as context) – keep this repository private, or
share it only with your team.
