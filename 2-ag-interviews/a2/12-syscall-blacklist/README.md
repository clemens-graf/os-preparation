# A2-12 · syscall blacklist

**Time box 20 min · ★☆☆**

```
Name:         blacklistsys
Nr:           1795
Description:  blacklist(n): the calling process can never use syscall n again - it
              returns -1 without being executed. exit can not be blacklisted.
Return Value: 0 (also if already blacklisted), -1 (exit, or list full: 16 entries)
```

## Test

`task.sh start a2-12` → `task.sh test`. Done when `sched_yield` returns -1 after blacklisting it,
and nothing is printed after `write` was blacklisted.

## Hints

<details><summary>1 – where</summary>

Top of `Syscall::syscallException`, before the switch.
</details>

<details><summary>2 – locking</summary>

Per-process list + Mutex. Even the check (a read) takes it: another thread may be adding an entry at the same time.
</details>

## Questions a tutor might ask

- Why must exit stay allowed?
- What about a child created after blacklisting (inherit?)
- Linux's version of this: seccomp.
