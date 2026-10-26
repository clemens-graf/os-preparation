# A0-13 · atexit

**Time box 20 min · ★☆☆ · type U (user space only) – the real A0 task of 14 Oct 2026 · read first: [TRIAGE.md](../../TRIAGE.md)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §5 exit & atexit · example: [ex14](../../examples/ex14-libc-function/)

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         atexit (stdlib.h)
Description:  atexit(function) registers a function that exit() calls. The functions run
              in REVERSE order of registration. Returning from main calls exit()
              (via _start). _exit() does NOT run them. At most 32.
Return Value: 0, or -1 (NULL, or too many)
```

## Test

`task.sh start a0-13` → `task.sh test`. Done when 6 checks pass.

## Level 2 – hints

<details><summary>1 – kernel or user space?</summary>

Pure libc: an array in `stdlib.c`, called from `exit()` in `exec.c` before the exit syscall.
(A kernel version would push the functions as upcalls – A0-07 – at the exit syscall.)
</details>

<details><summary>2 – threads</summary>

A plain array + counter is enough. If two threads register at the very same moment, both can read the same counter value and one function is lost – mention it; a fix is a lock (A1-15) or `__sync_fetch_and_add(&count, 1)` to reserve the slot.
</details>

<details><summary>3 – a handler that calls exit()</summary>

Pop each function before calling it: a nested exit() continues with the remaining ones instead of running them twice.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Your team's `main` already has your `atexit`. To practise: on the task branch replace the body of `atexit` in `stdlib.c` by `return -1;`, delete `__runAtexitFunctions` and its call in `exit()` (`exec.c`), commit.
2. In `stdlib.c`: a static array of 32 function pointers and a static counter.
3. `atexit(f)`: refuse NULL and a full array (-1); otherwise store `f` at the counter's position, increase the counter, return 0.
4. A run function: while the counter is above 0, decrease it **first**, then call the function at that position (reverse order; a handler that calls `exit` again does not run the same one twice).
5. In `exit()` (`exec.c`): call the run function **before** the exit syscall. `_exit()` stays as it is (no handlers).
6. `task test`, then `task diff a0-13` against your team's version.

</details>

## Your team's code

**Your team has it now – your own A0 implementation, merged with `feature/atexit`** (18 Oct), so
this is a **study task**: `task start a0-13` gives you `main` with your `atexit`, and `task test`
runs the 6 course checks against it (all pass). Explain it as if the tutor asked:
`atexit_functions[ATEXIT_MAX]` + `atexit_count` in `userspace/libc/src/stdlib.c`,
`__runAtexitFunctions()` (pops each function before calling it), called from `exit()` in
`userspace/libc/src/exec.c` before the exit syscall; `_exit()` skips it; `_start` in `nonstd.c`
is `exit(main())`. A plain counter, no atomics – fine as long as threads do not register at the
same moment (say so if the tutor asks).

**To write it again from scratch** (the plan's day 4): on your task branch, replace the body of
`atexit` by `return -1;`, delete `__runAtexitFunctions` and its call in `exit()`, commit – then
build it again in the time box. `task diff a0-13` afterwards compares with your team's version.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why reverse order?</summary>

Later registrations may depend on earlier ones (a library registered first, code that uses it later). Cleaning up in reverse order – like a stack – tears things down in the opposite order they were set up.

</details>

<details><summary>What is the difference between exit and _exit?</summary>

`exit` runs the `atexit` handlers (and would flush stdio buffers), then makes the exit syscall. `_exit` makes the syscall **directly** – no handlers. In your team's libc both are in `exec.c`; `exit` calls `__runAtexitFunctions()` first.

</details>

<details><summary>How would the kernel version look (A0-07 upcalls)?</summary>

The exit syscall would, before ending the process, divert the thread into each registered function like an upcall (A0-07): push the old `rip`, set `rip` = handler – and after the last one make the real exit. Much more complicated, and nothing in it needs the kernel – which is why the real answer is libc.

</details>
