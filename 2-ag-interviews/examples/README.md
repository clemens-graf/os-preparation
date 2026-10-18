# Examples – worked reference changes

Small, complete changes to your team's code, each one showing **one mechanism** you need in the
interview. They are references: read them, apply them, run them, change them. The tasks in
`a0/`–`a2/` are similar but not the same – the examples show the *how*, the tasks make you
find the *where* yourself.

| # | Example | Shows | Needed for |
|---|---|---|---|
| ex01 | [syscalls](ex01-syscalls/) | the 6 places of a new syscall, return values, copying to a user buffer, pointer checks | everything |
| ex02 | [map a page](ex02-map-page/) | `allocPPN`, ident mapping, `mapPage`, the page-table lock | A0-02, A0-04, A1 shm, A2 |
| ex03 | [stack growth](ex03-stack-growth/) | hooking a new case into the page fault handler | A0-03, A0-04, A0-06, A2 |
| ex04 | [PTE flags](ex04-pte-flags/) | `resolveMapping`, changing `writeable`, TLB flush | A0-04, A0-05, A2 mprotect / W⊕X |
| ex05 | [registers](ex05-registers/) | changing `user_registers_`: `rip`, `rsp`, `rdi`; red zone, alignment | A0-06, A0-07, A1 snapshot |
| ex06 | [kernel thread](ex06-kernel-thread/) | a `Thread` subclass, `addNewThread`, Mutex + Condition handshake, object lifetime | A0-08, A1, A2 swap thread |
| ex07 | [F-key](ex07-fkey/) | a key press in the Console thread, handled by the next syscall in the right process | A2 "press F3/F7" tasks |
| ex08 | [block device](ex08-block-device/) | reading/writing the swap partition `idea2`, global locks (no global constructors!) | A2 |
| ex09 | [loader](ex09-loader/) | the ELF program headers of the running process | A0-05, A2 W⊕X |
| ex11 | [tutorial 1](ex11-tutorial1/) | the first A0 tutorial as written there: a user string into the kernel, a debug flag of its own, per-thread data with a Mutex, `ScopeLock` – and its bugs | A0-11, A0-01 |
| ex12 | [tutorial 2](ex12-tutorial2/) | the A0 tutorial of 7 Oct as written there: `resolveMapping`, a frame mapped twice, why the kernel panics at exit, what a tutor would criticise | A0-12, A1-09, A1-10 |
| ex13 | [timer & scheduler](ex13-timer-scheduler/) | counting in the timer interrupt (`irqHandler_0`), changing the choice in `schedule()`, why no locks there | A1-04, A1-05, A1-07, A1-18 |
| ex14 | [a libc function](ex14-libc-function/) | only user space: declaration in the header, implementation in the libc, state inside the libc, `void*` + function pointers, name clashes | A0-13, A0-14, A1-14 … A1-17 |

## Tasks that share a mechanism

Many tasks are the same mechanism in a new place. Once you can do one, the others are mostly
"where" plus one new idea. Example: **ex03 → A0-03 → A1-12.**

| Step | What it does in the page fault handler |
|---|---|
| ex03 stack growth | the fault is just below the stack: map a fresh page, return – the instruction runs again |
| A0-03 heaplimit | the fault is in `[heap start, heap start + limit)`: map a fresh page – the limit is a constant |
| A1-12 sbrk | the same, but the end moves: `brk_` per process, set by a syscall; shrinking unmaps |
| A2-13 mmap | the same, for a range a syscall reserved |

All four are the same code: *one new `if` in `handlePageFault` → `allocPPN` → `mapPage` under
`thread_lock_`*. Only the condition changes. The groups:

| Mechanism | Example | Tasks |
|---|---|---|
| new case in the page fault handler | ex03 | A0-03, A0-04, A1-12, A2-13, A0-06 |
| map / unmap a frame | ex02, ex12 | A0-02, A0-12, A1-10, A2-09 |
| walk the page tables | ex12 | A0-01, A1-09 |
| change a thread's saved registers | ex05 | A0-07, A0-06, A1-08, A1-13 |
| count in the timer, decide in `schedule()` | ex13 | A1-04, A1-05, A1-07, A1-18 |
| sleep until another thread signals (Mutex + Condition) | ex06 | A1-01, A1-02, A1-03, A1-19 |
| libc only | ex14 | A0-13, A0-14, A1-14 … A1-17 |
| a key press reaches a process | ex07 | A2-06, A2 F-key tasks |
| the disk | ex08 | A2-01 … A2-05, A2-11 |

Theory behind each example: [concepts.pdf](../concepts.pdf) – every section ends with the examples that
show it in code.

## Using an example

```bash
task example ex01         # branch example/ex01-syscalls in repos/sweb-ag, example applied, test copied
task test                 # build + run its test
git -C ~/Documents/University/2026-fall/os/repos/sweb-ag show --stat   # which files it changed
task run                  # boot it and play with it
task reset ex01           # back to the unchanged example
```

Each example applies on its own to a clean `main` (they do not depend on each other).
`example.patch` is the change on your team's code (the commit in `team/BASE`).
Syscall numbers 1400–1499 (except ex11 and ex12: the tutorials' 142/143, and 1580 for tutorial 1 –
its original 1000 is your team's `pthread_create`). There is no ex10: the tutorial examples carry the number of their task (11, 12). `example.patch` is plain `git diff` output – reading it is the
fastest way to see *which line goes where*. ex07 has no test program: `task test` presses F8 and
shows the `[FKEY]` line the kernel prints. ex11 and ex12 demonstrate bugs of the tutorial code: ex11's
second program is killed by a General Protection Fault, ex12's run ends in a kernel panic –
`task test` counts exactly that as success (`--expect-panic`).
