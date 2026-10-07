# A1-14 · atexit

**Time box 20 min · ★☆☆ · man page: atexit**

```
Name:         atexit (stdlib.h - the stub exists)
Description:  atexit(function) registers a function that exit() calls. The functions run
              in REVERSE order of registration. Returning from main calls exit()
              (via _start). _exit() does NOT run them. At most 32.
Return Value: 0, or -1 (NULL, or too many)
```

## Test

`task.sh start a1-14` → `task.sh test`. Done when 6 checks pass.

## Hints

<details><summary>1 – kernel or user space?</summary>

Pure libc: an array in `stdlib.c`, called from `exit()` in `exec.c` before the exit syscall.
(A kernel version would push the functions as upcalls – A0-07 – at the exit syscall.)
</details>

<details><summary>2 – threads</summary>

Two threads registering at once race on the counter – reserve the slot with an atomic increment.
</details>

<details><summary>3 – a handler that calls exit()</summary>

Pop each function before calling it: a nested exit() continues with the remaining ones instead of running them twice.
</details>

## Questions a tutor might ask

- Why reverse order?
- What is the difference between exit and _exit?
- How would the kernel version look (A0-07 upcalls)?
