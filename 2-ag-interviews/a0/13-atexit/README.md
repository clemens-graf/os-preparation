# A0-13 · atexit

**Time box 20 min · ★☆☆ · type U (user space only) – the real A0 task of 14 Oct 2026 · read first: [TRIAGE.md](../../TRIAGE.md)**

```
Name:         atexit (stdlib.h - the stub exists)
Description:  atexit(function) registers a function that exit() calls. The functions run
              in REVERSE order of registration. Returning from main calls exit()
              (via _start). _exit() does NOT run them. At most 32.
Return Value: 0, or -1 (NULL, or too many)
```

## Test

`task.sh start a0-13` → `task.sh test`. Done when 6 checks pass.

## Your team's code

Pure libc – nothing in your team's kernel changes. The stub is in `userspace/libc/src/stdlib.c`, `exit()`/`_exit()` in `userspace/libc/src/exec.c`, `_start` (`exit(main())`) in `nonstd.c`. The team solution uses a plain counter (no atomics) – fine as long as threads do not register at the same moment (say so if the tutor asks).

## Hints

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

## Questions a tutor might ask

- Why reverse order?
- What is the difference between exit and _exit?
- How would the kernel version look (A0-07 upcalls)?
