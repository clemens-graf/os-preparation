# A1 – interview tasks

The A1 interview is about **threads, scheduling, processes and syscalls** – and the task
list from earlier years (`AG - Potential Tasks.md`) says which kinds of tasks come up. In
the interview you work in your team's A1 implementation; here you train in base SWEB.
Tasks that need threads build on a small multithreading base: the A0-08 solution
(`pthread_create`) plus A1-01 (`pthread_join`) – `task.sh start` sets that up for you.

**A1-01 and A1-06 are your own P2 work** (`pthread_join`, `pthread_cancel`) – do them
first: they are the best preparation for the assignment *and* for the interview.

## The tasks

| # | Task | From the list | Builds on | Time box | Level |
|---|---|---|---|---|---|
| 01 | [pthread_join](01-pthread-join/) | (mandatory A1) | A0-08 | 60 min | ★★★ |
| 02 | [pthread_multiple + pthread_join_any](02-pthread-multiple/) | `pthread_multiple` | 01 | 45 min | ★★★ |
| 03 | [barriers](03-barrier/) | `pthread_barriers` | 01 | 40 min | ★★★ |
| 04 | [thread flagging: twice as often](04-thread-flag/) | *thread-flagging* | 01 | 30 min | ★★☆ |
| 05 | [pthread_invoke: directed yield](05-pthread-invoke/) | `pthread_invoke`, `pthread_switch` | 01 | 30 min | ★★☆ |
| 06 | [pthread_cancel (deferred)](06-pthread-cancel/) | (mandatory A1) | 01 | 45 min | ★★★ |
| 07 | [thread runtime + clock()](07-thread-runtime/) | `clock`-extension | 01 | 30 min | ★★☆ |
| 08 | [snapshot & revive](08-snapshot/) | `pthread_snapshot & revive` | – | 40 min | ★★★ |
| 09 | [vtop / ptov](09-vtop-ptov/) | *vpn to ppn and ppn to vpn* | – | 25 min | ★★☆ |
| 10 | [a page shared by two processes](10-shared-page/) | *simple shm* | – | 45 min | ★★★ |
| 11 | [page fault statistics + cr2](11-pagefault-stats/) | *x86 asm pagefault* | – | 30 min | ★★☆ |
| 12 | [sbrk](12-sbrk/) | *simple sbrk* | – | 30 min | ★★☆ |
| 13 | [a 15-argument syscall](13-sum15/) | *15 arg syscall* | – | 15 min | ★☆☆ |
| 14 | [atexit](14-atexit/) | `atexit` | – | 20 min | ★☆☆ |

Not here, with the reason:
- *Pagefault handler with special addresses* (`0xDEADBEEF`): that is A0-04 – do it again in
  the team repo with a variant (kill only the thread, print its registers).
- `forkall` (fork with all threads): needs a working `fork` – practise it in the team repo
  once Team F's fork is merged. The mechanics: copy every thread's registers and stack
  slot into the child, all under the page-table lock of the parent.

## How to work on a task

```bash
task.sh start a1-01        # branch task/a1-01-pthread-join, with the base and the tests
# implement (task card: a1/01-pthread-join/README.md)
task.sh test               # build + run the tests headless
task.sh run                # boot it in a window, like sweb-practise
task.sh reset a1-01        # start over - your attempt is kept in a backup branch
task.sh solution a1-01     # the reference solution on its own branch
```

`task.sh` is in `tools/`; see the main README for the setup. Syscall numbers 1600–1719.

## What A1 interview questions are about

Every thread task comes down to the same five questions – be able to answer them for your
own team's code:

1. Where are the threads of a process listed, and **which lock** protects that list?
2. When is a thread's object freed, and how do you make sure nobody uses it afterwards?
3. How does a thread wait for something – which Mutex, which Condition, which state?
4. What can happen between two lines (a timer interrupt, another thread finishing)?
5. What does the scheduler see, and why can it not take any lock?
