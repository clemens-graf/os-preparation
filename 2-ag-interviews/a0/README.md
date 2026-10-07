# A0 – interview tasks

## What students reported

1. "mapping some memory using a syscall, something to do with pagefaults or register manipulation"
2. "confined to pagefault interrupts and syscalls – make sure you know how to add to both.
   Most likely memory mapping: know how to allocate a page and map it for the user."
   "The mechanics behind `pthread_create()` – try it and know what you did and where."
3. "Know your way around the code. E.g. when a segfault happens, a function the user
   registered before should run first: you need to know where segfaults are handled,
   implement a syscall to set the function, …"
4. "something with the loader"
5. "something with syscalls and something with pages"
6. "We had to do `pthread_multi`: it gets a function and argument list, and one thread
   executes all these functions in serial."
7. The A0 preparation tutorials, done by the tutors themselves:
   - **tutorial 1:** a syscall that takes a user string, a debug flag of its own, per-thread
     data with a Mutex. Reproduced in [ex11](../examples/ex11-tutorial1/); A0-11 is the same
     task done right.
   - **tutorial 2** (7 Oct): `get_physical_address` and `map_address` – the physical page behind
     a variable, and the same page mapped at a second address. Reproduced in
     [ex12](../examples/ex12-tutorial2/); A0-12 is the same task done right.

## The tasks

| # | Task | Hint it trains | Time box | Level | Example to read first |
|---|---|---|---|---|---|
| 01 | [meminfo](01-meminfo/) – syscall with a user struct, page-table walk | 1, 2, 5 | 25 min | ★☆☆ | ex01 |
| 02 | [allocpage / freepage](02-allocpage/) – map and unmap pages for the user | 1, 2, 5 | 30 min | ★★☆ | ex02 |
| 03 | [heaplimit](03-heaplimit/) – a heap that the page fault handler maps on demand | 1, 2 | 30 min | ★★☆ | ex03 |
| 04 | [magic page](04-magic-page/) – a special address in the page fault handler, read-only | 2, 5 | 25 min | ★★☆ | ex03, ex04 |
| 05 | [read-only code](05-readonly-code/) – the loader maps segments with their permissions | 4 | 25 min | ★★☆ | ex09, ex04 |
| 06 | [segv handler](06-segv-handler/) – call a user function on a segfault | 3, 1 | 40 min | ★★★ | ex05, ex03 |
| 07 | [upcall](07-upcall/) – register manipulation: call a user function, then continue | 1 | 30 min | ★★☆ | ex05 |
| 08 | [pthread_create](08-pthread-create/) – the mechanics: stack, registers, address space | 2 | 60 min | ★★★ | ex06, ex02 |
| 09 | [pthread_multi](09-pthread-multi/) – one thread runs a list of functions | 6 | 40 min | ★★★ | (builds on 08) |
| 11 | [a note per thread](11-thread-note/) – tutorial 1 done right: a user string of unknown length, per-thread data, a debug flag | 7, 1 | 30 min | ★★☆ | ex11, ex01 |
| 12 | [two addresses, one page](12-alias-page/) – tutorial 2 done right: physical page lookup, aliases, no double free | 7, 1, 5 | 40 min | ★★★ | ex12, ex02 |

There is no task 10: the two tutorial tasks carry the number of their example –
11 = tutorial 1 (ex11), 12 = tutorial 2 (ex12).

The time boxes are for the second pass, when you know the code. The first time, take as
long as you need – but keep a note of where you lost the time.

## How to work on a task

```bash
task start a0-01          # branch task/a0-01-meminfo in repos/sweb-ag, tests already in userspace/tests/
# implement - task card: a0/01-meminfo/README.md
task test                 # build + run the tests headless (with leak check)
task run                  # or boot it in a window and type meminfo_basic.sweb
task reset a0-01          # start over from scratch (your attempt is kept in a backup branch)
task solution a0-01       # the reference solution on its own branch; task diff a0-01 compares
```

(`task` = `tools/task.sh`, see the main README.) A0-09 starts from the A0-08 solution –
`task start a0-09` sets that up.

## Each task folder contains

- `README.md` – the task card in the tutors' format (name, description, parameters,
  return value, notes), how to test, hints behind fold-outs, and questions a tutor might
  ask afterwards;
- one or more test programs (`task start` copies them into `userspace/tests/`).

The solution is in [`../solutions/a0/`](../solutions/a0/) – `NOTES.md` explains it step by step.

## Conventions used in all tasks

- Syscall numbers: A0 uses 1500–1599 (your team block), the examples 1400–1499.
- Return `-1` on error as `(size_t) -1` in the kernel – **not** `-1U` (that is
  `0xFFFFFFFF`, which user space does not see as `-1`).
- The libc wrappers go into `userspace/libc/src/nonstd.c` and
  `userspace/libc/include/nonstd.h` (pthread functions into `pthread.c`/`pthread.h`).
  `nonstd.h` does not include `types.h` – add it, or `size_t` is unknown there.
- Code style as in the team: 2 spaces, functions camelCase, variables snake_case,
  syscall macros `sc_lowercase`.
