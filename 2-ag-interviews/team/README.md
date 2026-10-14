# Your team's code – what the course runs on

In the interview you work in **your team repo**, so the whole course runs on it: every task and
every example starts from a snapshot of your team's `main` (`osw26e3`), and the cards, hints,
notes and solutions use your team's classes and names.

All solutions (`../solutions/*/*/solution.patch`) and examples (`../examples/*/example.patch`)
were made and tested on the commit in [`BASE`](BASE) – `osw26e3` main of 13 Oct 2026, `40140475`
(with `pthread_create`, `pthread_join`, `pthread_cancel`, `pthread_self`, `fork`, `execv`, the
growing stack). Every one of them builds and passes its tests there – except the two study tasks
that show bugs in your code (A1-01, A1-06, below).

## How it works

```bash
task start a0-01       # -> branch task/a0-01-meminfo, on your team's code
task test
task solution a0-01    # -> branch solution/a0-01-meminfo
```

- The first `task` command imports that commit from `repos/osw26e3` as branch `main` of the
  training copy. Your team repo is only read; nothing there changes, and the training copy has
  no remote. When `team/BASE` moves to a newer commit, `main` moves with it, and `task start`
  warns about task branches that were started on the older code (`task reset <id>` starts them
  again – your attempt is kept as a backup).
- `main` is your team's code; your work is on `task/...`, `solution/...`, `example/...`,
  `base/...` (a task that builds on another one), `backup/...`.
- Study tasks (A0-08, A1-01, A1-06): your team already has the feature – the solution patch is
  empty, `task test` runs the course tests against your code.

## The names you use

| What | Your team's code |
|---|---|
| the calling process | `currentThread->getProcess()` (a `UserProcess*`) |
| its page tables | `process->getAddressSpace()` (an `ArchMemory&`, owned by the `UserProcess`; `~UserProcess` deletes it) |
| the page-table lock | `process->getThreadLock()` – the per-process Mutex `thread_lock_`; also protects `threads_` and the stack slots. The page fault handler holds it for stack pages, **not** around `loadPage` |
| the ELF loader | `process->getLoader()` (a `Loader&`): `loadPage(address, getAddressSpace())` |
| per-process state | members of `UserProcess` |
| per-thread state | members of `UserThread` – `(UserThread*) currentThread` |
| a thread by id | `process->findThreadDataByTid(tid)` (hold `thread_lock_`) → `UserThreadData{thread_, exit_value_, tid_, exited_, joined_, requested_cancel_}`; `thread_` is `nullptr` once it exited |
| stacks | a slot per thread (`USERSTACK_PAGES` 4 + a guard page), grown in the page fault handler (`isUserStackFault`) |
| syscall numbers taken | 1000–1002 `pthread_create/exit/self`, 1500 `mkdir`, 1501–1503 `pthread_join/cancel/testcancel`, 1998/1999 counts, 2000–2002 `fork/getpid/getppid`, 2500 `execv` |

The tutorials (ex11, ex12) were written for upstream SWEB, where `UserProcess` *is* a `Thread`.
When you read such code: `currentThread->loader_` / `(UserProcess*) currentThread` →
`currentThread->getProcess()`; `loader_->arch_memory_` → `getAddressSpace()`; the cast still
compiles in your code, but reads garbage.

## All tasks – where the code goes

Types: **U** user space only · **S** syscall + kernel · **E** kernel event · **A** assembly/registers ·
**K** kernel calls user code – see [`../TRIAGE.md`](../TRIAGE.md); files: [`../ATLAS.md`](../ATLAS.md).

| Task | Type | In your team's code |
|---|---|---|
| A0-01 meminfo | S | 1505; `ArchMemory::countUserPages()` under `getThreadLock()` |
| A0-02 allocpage | S | `getAddressSpace()` + `getThreadLock()` |
| A0-03 heaplimit | S + E | `heap_size_`, `mapHeapPage()`, `setHeapLimit()` in `UserProcess`; heap case first in the page fault handler's valid branch |
| A0-04 magic page | E + S | `UserProcess::mapReadOnlyPage()`; magic case first |
| A0-05 readonly code | E | `ArchMemory::mapPage(…, writeable)` – read-only in one step (`loadPage` runs without the lock) |
| A0-06 segv handler | K + E | `segv_*` + `deliverSegv()` in `UserProcess`; `loadPage` returns `bool` |
| A0-07 upcall | K | `checkAddressValid` under `getThreadLock()` |
| A0-08 pthread_create | S | **study task** – your team's own (8/9: the 9th is a different exit design) |
| A0-09 pthread_multi | S + K | `createThread(entry, false, 0, 0)` + `UserThread::pushToStack()` |
| A0-11 thread note | S | `char note_[16] = {}` in `UserThread` |
| A0-12 alias page | S | alias list in `UserProcess`, cleanup in `~UserProcess` before `delete arch_memory_` |
| A1-01 pthread_join | S | **study task** – your P2 work: 8/9, then a kernel panic (below) |
| A1-02 pthread_multiple | S | adds `MAX_USER_THREADS 64` to `createThread`; `discardThread`, `joinAny` in `UserProcess` |
| A1-03 barrier | S | `barriers_`, `barriers_lock_`, `barrier_opened_` in `UserProcess` |
| A1-04 thread flag | E + S | `double_slice_` in `Thread`, `Scheduler::schedule()` |
| A1-05 pthread_invoke | E + S | `Scheduler::preferred_` + `yieldTo()` |
| A1-06 pthread_cancel | S | **study task** – your P2 work: 2 pass, then it hangs (below) |
| A1-07 thread runtime | E + S | `cpu_ticks_` in `Thread` (`incTicks`), `finished_ticks_` in `UserProcess` |
| A1-08 snapshot | A + S | `snapshot_` in `UserThread`; the stack top is the thread's own slot |
| A1-09 vtop/ptov | S | `ArchMemory::findVirtualPage()` |
| A1-10 shared page | S + E | `SharedPages`; `shm_attached_` in `UserProcess`; `unmapPage(vpn, false)` |
| A1-11 pagefault stats | A + E | `pf_stats_` in `UserProcess`, counted in `pageFaultHandler` |
| A1-12 sbrk | S + E | like A0-03: `brk_`, `mapBrkPage()`, `sbrk()` in `UserProcess` |
| A1-13 sum15 | A | only the six places |
| A1-14 atexit | U | libc: `stdlib.c` + `exit()` in `exec.c` |
| A1-15 malloc | U | libc: `stdlib.c` |
| A1-16 spinlock + mutex | U | libc: `pthread.c` (+ the missing declarations in `pthread.h`) |
| A1-17 semaphore | U | libc: `semaphore.c` |
| A2-01 swap | S + E | `SwapManager`; `swapOut/swapIn` in `UserProcess`; swap-in first in the page fault handler |
| A2-02 … A2-06 | E (+ S) | on top of A2-01, state in `UserProcess`; A2-06: `Console::handleKey` + top of `syscallException` |
| A2-07 mprotect | S + E | PTE bits under `getThreadLock()` |
| A2-08 NX | E | `loadPage` + `UserProcess::mapStackPage` |
| A2-09 zero page | E | `mapPage(…, writeable)` + `UserProcess::copyOnWrite()` |
| A2-10 RAM obfuscation | E | `all_processes_` list, `relocatePage/isMapped` in `UserProcess`, `ObfuscationThread` |
| A2-11 boot counter | E + S | `BootRecord`, `init()` in `ProcessRegistry::Run()` |
| A2-12 syscall blacklist | E + S | `blacklist_` in `UserProcess`, checked at the top of `syscallException` |
| ex03 stack growth | E | your stack already grows – the example raises `USERSTACK_PAGES` to 16 |
| ex11 / ex12 | S | the tutorials, moved to your team's code (1580 / `getProcess()->getAddressSpace()`) |

## Found in your team's code – worth knowing (and fixing)

- **A1-01: a thread sleeping in `pthread_join` crashes the kernel when the process exits.**
  `exitProcess` calls `kill()` on it, but it stays in the waiters list of `thread_exited_`;
  `~UserProcess` destroys the Condition → "still waiting on this lock" → panic. Fix:
  `thread_exited_.broadcast()` in `exitProcess` before the kill loop
  ([notes](../solutions/a1/01-pthread-join/NOTES.md)).
- **A1-06: a thread sleeping in `pthread_join` cannot be cancelled.** `cancelThread` only sets
  the flag – nobody wakes the sleeper, and the join loop never checks the flag. Fix: broadcast
  in `cancelThread`, check the caller's flag in the wait loop, end in the syscall layer
  ([notes](../solutions/a1/06-pthread-cancel/NOTES.md)).
- **`thread_lock_` is the page-table lock** – the page fault handler takes it for stack pages,
  but **releases it before `loadPage`**, so a page loaded from the binary is mapped without it.
  Two threads faulting on the same page: `mapPage` returns `false` for the second, which then
  frees its frame – correct, but a tutor may ask about it.
- **`exit` / returning from `main` ends the whole process** (`exitProcess` kills all threads) –
  POSIX behaviour.
- **Many POSIX functions are stubs in your libc** (`malloc`, `atexit`, `sem_*`, `pthread_mutex_*`,
  `pthread_spin_*`, `sleep`, `clock`, `waitpid`, `mmap`, …) – a task may ask for any of them:
  [ATLAS.md section 5](../ATLAS.md).
- **`Syscall::mkdir` starts with `size_t result = -1U;`** – as a `size_t` that is `0xFFFFFFFF`,
  not -1. It only works because the `int` wrapper cuts it back to 32 bits.
- **`Syscall::copyUserString()`** exists (byte by byte, stops at `USER_BREAK`) – reuse it where a
  string must fit completely.
- **No thread limit**: `createThread` accepts threads until memory runs out (A1-02 adds one).
- GRUB boots without waiting (`timeout 0` in `menu.lst`) – `task run` starts SWEB directly.

## When your team's main moves on

`team/BASE` pins the commit; the patches are made and tested on it. To practise on a newer team
state, the patches have to be re-made on that commit and `BASE` updated – then `task` moves
`main` by itself and warns about old task branches.

These patches contain lines of your team's code (as context).
