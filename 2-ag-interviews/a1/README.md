# A1 – interview tasks

The A1 interview is about **threads, scheduling, processes and syscalls** – and the task
list from earlier years (`AG - Potential Tasks.md`) says which kinds of tasks come up. Every task
runs on **your team's code** (the commit in `team/BASE`) – the code you have in the interview.
A1-01 and A1-06 are your team's own `pthread_join` / `pthread_cancel` (your P2 work): study tasks
– the course tests run against your code (and find two bugs, see their notes). A1-14 … A1-20 cover every POSIX stub of your libc that
an A1 task can ask for.

**Not every task is a syscall:** A1-14 … A1-17 are pure libc (type U) – like the `atexit` task in
A0 of 2026 (that one is [A0-13](../a0/13-atexit/)). Before you start a task, classify it: [TRIAGE.md](../TRIAGE.md).

## The tasks

| # | Task | Type | From the list | Builds on | Time box | Level |
|---|---|---|---|---|---|---|
| 01 | [pthread_join](01-pthread-join/) | S (study) | (mandatory A1) | A0-08 | 60 min | ★★★ |
| 02 | [pthread_multiple + pthread_join_any](02-pthread-multiple/) | S | `pthread_multiple` | 01 | 45 min | ★★★ |
| 03 | [barriers](03-barrier/) | S | `pthread_barriers` | 01 | 40 min | ★★★ |
| 04 | [thread flagging: twice as often](04-thread-flag/) | E + S | *thread-flagging* | 01 | 30 min | ★★☆ |
| 05 | [pthread_invoke: directed yield](05-pthread-invoke/) | E + S | `pthread_invoke`, `pthread_switch` | 01 | 30 min | ★★☆ |
| 06 | [pthread_cancel (deferred)](06-pthread-cancel/) | S (study) | (mandatory A1) | 01 | 45 min | ★★★ |
| 07 | [thread runtime + clock()](07-thread-runtime/) | E + S | `clock`-extension | 01 | 30 min | ★★☆ |
| 08 | [snapshot & revive](08-snapshot/) | A + S | `pthread_snapshot & revive` | – | 40 min | ★★★ |
| 09 | [vtop / ptov](09-vtop-ptov/) | S | *vpn to ppn and ppn to vpn* | – | 25 min | ★★☆ |
| 10 | [a page shared by two processes](10-shared-page/) | S + E | *simple shm* | – | 45 min | ★★★ |
| 11 | [page fault statistics + cr2](11-pagefault-stats/) | A + E | *x86 asm pagefault* | – | 30 min | ★★☆ |
| 12 | [sbrk](12-sbrk/) | S + E | *simple sbrk* | – | 30 min | ★★☆ |
| 13 | [a 15-argument syscall](13-sum15/) | A | *15 arg syscall* | – | 15 min | ★☆☆ |
| 14 | [malloc / free / calloc / realloc](14-malloc/) | **U** | libc stub | – | 30 min | ★★☆ |
| 15 | [spinlock + mutex in user space](15-spinlock/) | **U** | libc stub | 01 | 25 min | ★★☆ |
| 16 | [semaphores in user space](16-semaphore/) | **U** | libc stub | 15 | 25 min | ★★☆ |
| 17 | [condition variables in user space](17-cond/) | **U** | libc stub | 15 | 25 min | ★★☆ |
| 18 | [sleep + uptime](18-sleep/) | S + E | libc stub | – | 25 min | ★★☆ |
| 19 | [waitpid](19-waitpid/) | S | libc stub (processes) | – | 40 min | ★★★ |
| 20 | [pthread_detach](20-detach/) | S | libc stub | – | 25 min | ★★☆ |

Not here, with the reason:
- *Pagefault handler with special addresses* (`0xDEADBEEF`): that is A0-04 – do it again with a
  variant (kill only the thread, print its registers).
- `forkall` (fork with all threads): your team's `fork` (`UserProcess::fork`) copies only the
  calling thread. The mechanics: copy every thread's registers and stack slot into the child,
  all under `thread_lock_` of the parent.

## How to work on a task

```bash
task start a1-01        # branch task/a1-01-pthread-join, with the base and the tests
# implement (task card: a1/01-pthread-join/README.md)
task test               # build + run the tests headless
task run                # boot it in a window, like sweb-practise
task reset a1-01        # start over - your attempt is kept in a backup branch
task solution a1-01     # the reference solution on its own branch
```

Syscall numbers 1600–1719 (your block 1500–1999; 1500–1503 and 1998/1999 are taken).

## What A1 interview questions are about

Every thread task comes down to the same five questions – be able to answer them for your
team's code:

1. Where are the threads of a process listed, and **which lock** protects that list?
2. When is a thread's object freed, and how do you make sure nobody uses it afterwards?
3. How does a thread wait for something – which Mutex, which Condition, which state?
4. What can happen between two lines (a timer interrupt, another thread finishing)?
5. What does the scheduler see, and why can it not take any lock?
