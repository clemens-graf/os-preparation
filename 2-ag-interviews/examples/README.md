# Examples – worked reference changes

Small, complete changes to base SWEB, each one showing **one mechanism** you need in the
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

## Using an example

```bash
task example ex01         # branch example/ex01-syscalls in repos/sweb-ag, example applied, test copied
task test                 # build + run its test
git -C ~/Documents/University/2026-fall/os/repos/sweb-ag show --stat   # which files it changed
task run                  # boot it and play with it
task reset ex01           # back to the unchanged example
```

Each example applies on its own to a clean `main` (they do not depend on each other).
Syscall numbers 1400–1499 (except ex11 and ex12, which keep the tutorials' numbers 1000 and
142/143). There is no ex10: the tutorial examples carry the number of their task (11, 12). `example.patch` is plain `git diff` output – reading it is the
fastest way to see *which line goes where*. ex07 has no test program: `task test` presses F8 and
shows the `[FKEY]` line the kernel prints. ex11 and ex12 demonstrate bugs of the tutorial code: ex11's
second program is killed by a General Protection Fault, ex12's run ends in a kernel panic –
`task test` counts exactly that as success (`--expect-panic`).
