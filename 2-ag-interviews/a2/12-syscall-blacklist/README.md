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

## Your team's code

Per process → `UserProcess`: `blacklist_[16]`, `blacklist_count_`, its own `Mutex blacklist_lock_{"…"}` (default member initializer). The check goes to the top of `Syscall::syscallException` – next to your team's cancellation check.

## Hints

<details><summary>1 – where</summary>

Top of `Syscall::syscallException`, before the switch.
</details>

<details><summary>2 – locking</summary>

Per-process list + Mutex. Even the check (a read) takes it: another thread may be adding an entry at the same time.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why must exit stay allowed?</summary>

If `exit` were blacklisted the process could never end: returning from `main` calls `exit()`, which would just return -1 – and `_start` has nothing after it. A kill from outside (a fault → `Syscall::exit`) still works, but a normal end would be impossible.

</details>

<details><summary>What about a child created after blacklisting (inherit?)</summary>

With `fork` the child is a copy of the parent's state – the blacklist (per process) should be copied too, else a restricted program escapes by forking. seccomp filters are inherited by children for exactly that reason. With `exec` the list should also stay (Linux: inherited across execve).

</details>

<details><summary>Linux's version of this: seccomp.</summary>

**seccomp** (Linux): a process installs a filter (strict mode: only `read`/`write`/`exit`/`sigreturn`; filter mode: a BPF program that decides per syscall and argument). Once installed it can't be removed, and it is inherited by children – used by browsers, Docker, systemd to sandbox code.

</details>
