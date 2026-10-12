# Your team's code – what the course runs on

In the interview you work in **your team repo**, not in base SWEB. So the course runs there by
default: every A0 task and every example starts from a snapshot of your team's `main`
(`osw26e3`), and the cards, hints, notes and reference solutions use your team's classes and names.

Everything in this folder was made and tested on the commit in [`BASE`](BASE)
(`osw26e3` main of 12 Oct 2026, `947d1418` – with `pthread_create`, `pthread_join`, `fork`, the
growing stack): all A0 tasks and examples build and pass their tests on it.

## How it works

```bash
task start a0-01       # -> branch task/a0-01-meminfo, on your team's code
task test
task solution a0-01    # -> branch solution/a0-01-meminfo
```

- The first `task` command imports that commit from `repos/osw26e3` as branch `main` of the
  training copy (`task team setup` does it explicitly). Your team repo is only read; nothing there
  changes. When `team/BASE` moves to a newer commit, `main` moves with it, and `task start` warns
  about task branches that were started on the older code (`task reset <id>` starts them again).
- `main` is your team's code; your work is on `task/...`, `solution/...`, `example/...`,
  `backup/...`. Base SWEB lives under `base-sweb/` (`base-sweb/main` = upstream SWEB).
- No version for your team's code (A1, A2, ex03): `task` uses base SWEB and says so.
- `task mode base` switches everything to base SWEB; `task mode team` back to the default.

## What is different in your team's code

| Base SWEB (base solutions) | Your team's code (team solutions) |
|---|---|
| `class UserProcess : public Thread` – the process *is* its only thread | `class UserProcess` (no thread) + `class UserThread : public Thread`; `UserProcess::create`, `createThread`, `fork` |
| `currentThread->loader_` | `currentThread->getProcess()` – then `getLoader()` (a `Loader&`) |
| the page tables: `loader_->arch_memory_` (in the `Loader`) | **`process->getAddressSpace()`** (an `ArchMemory&`, owned by the `UserProcess`; `~UserProcess` deletes it) |
| no page-table lock (the course adds `Loader::arch_memory_lock_`) | **`process->getThreadLock()`** – the per-process Mutex; the page fault handler holds it while it maps stack pages |
| `loader_->loadPage(address)` | `getLoader().loadPage(address, getAddressSpace())` – the page fault handler calls it **without** `thread_lock_` |
| `(UserProcess*) currentThread` | `currentThread->getProcess()` – the old cast still compiles, but reads garbage |
| per-process state in the `Loader` | per-process state in `UserProcess` |
| per-thread state in `Thread` | per-user-thread state in `UserThread`: `(UserThread*) currentThread` |
| one stack page; ex03 grows the stack | a stack slot per thread (`USERSTACK_PAGES` + a guard page), grown in the page fault handler (`isUserStackFault`) |
| the course's `pthread_create` (A0-08) | your team's `pthread_create`, `pthread_exit`, `pthread_join`, `fork` |
| syscall numbers as on the cards | 1000/1001 `pthread_create/exit`, 1500 `mkdir`, 1501 `pthread_join`, 2000–2002 `fork`/`getpid`/`getppid` → meminfo uses **1505**, the tutorial-1 example **1580** |

Stays the same: `ArchMemory` (`mapPage`, `unmapPage`, `resolveMapping`, `checkAddressValid`),
`PageManager`, `currentThread->user_registers_`, `Scheduler`, the six places of a syscall.

## Per task

| Task | Team version |
|---|---|
| A0-01 meminfo | number 1505; walk `getAddressSpace()` under `getThreadLock()` |
| A0-02 allocpage | `getAddressSpace()` + `getThreadLock()` |
| A0-03 heaplimit | `heap_size_`, `mapHeapPage()`, `setHeapLimit()` in `UserProcess`; the heap case first in the page fault handler's valid branch |
| A0-04 magic page | `UserProcess::mapReadOnlyPage()`; the magic case first in the valid branch |
| A0-05 readonly code | `ArchMemory::mapPage(…, writeable)` – read-only in one step (your `loadPage` runs without the lock) |
| A0-06 segv handler | `segv_*` + `deliverSegv()` in `UserProcess`; `loadPage` returns `bool`; one place in the page fault handler for "invalid or no segment" |
| A0-07 upcall | `getAddressSpace().checkAddressValid` under `getThreadLock()` |
| **A0-08 pthread_create** | **study task**: your team has it – `task test` runs the course tests against it (8/9; the 9th is a different exit design) |
| A0-09 pthread_multi | built on your team's `createThread(entry, false, 0, 0)` + `UserThread::pushToStack()` |
| A0-11 thread note | `char note_[16] = {}` in `UserThread` |
| A0-12 alias page | `getAddressSpace()`, `getThreadLock()`; alias list in `UserProcess`, cleanup in `~UserProcess` before `delete arch_memory_` |
| ex03 | runs on base SWEB – your team's stack already grows |
| ex11 | the tutorial's number 1000 is your `pthread_create`: 1580 |
| ex12 | `getProcess()->getAddressSpace()` instead of the tutorial's cast |
| other examples | `getAddressSpace()`/`getThreadLock()` instead of the loader |

Every card (`../a0/NN-*/README.md`) has a section **"Your team's code"** with the details for that
task.

## Found in your team's code – worth knowing before the interview

- **`thread_lock_` is the page-table lock** – the page fault handler takes it for stack pages, but
  **releases it before `loadPage`**, so a page loaded from the binary is mapped without it. Two
  threads faulting on the same page: `mapPage` returns `false` for the second, which then frees
  its frame – correct, but a tutor may ask about it.
- **`exit` / returning from `main` ends the whole process** (`exitProcess` kills all threads) –
  POSIX behaviour. The course's A0-08 waited for the threads instead.
- **`Syscall::mkdir` starts with `size_t result = -1U;`** – the tutorial-1 trap: as a `size_t`
  that is `0xFFFFFFFF`, not -1. It only works because the `int` wrapper cuts it back to 32 bits.
- **`Syscall::copyUserString()`** exists (byte by byte, stops at `USER_BREAK`) – reuse it where a
  string must fit completely; it refuses longer strings instead of cutting them (see A0-11).
- GRUB boots without waiting (`timeout 0` in `menu.lst`) – `task run` starts SWEB directly.

## When your team's main moves on

`team/BASE` pins the commit; the team solutions are made and tested on it. To practise on a newer
team state, the team patches have to be re-made on that commit and `BASE` updated (then `task`
moves `main` by itself and warns about old task branches).

These patches contain lines of your team's code (as context).
