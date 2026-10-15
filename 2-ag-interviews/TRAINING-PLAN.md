# Training plan – everything once, every type covered

If you work through this list, you have practised **every kind of task** the AG interviews can
throw at you – not only syscalls. Every task runs on your team's code (`task start <id>`).
Tick a line when the task passes **without looking at the solution**, in its time box, and you
can answer the card's tutor questions out loud.

Before each task: **30 seconds of triage** – type (U / S / E / A / K, [TRIAGE.md](TRIAGE.md)) and
the first file ([ATLAS.md](ATLAS.md)). Say both out loud. Then start the timer.

## 0 · Orientation (once, ~2 h)

- [ ] [TRIAGE.md](TRIAGE.md) and [ATLAS.md](ATLAS.md) sections 1–2 read
- [ ] `task quiz 20` – twice, on different days (44 questions: places in the code + "which type?")
- [ ] guide chapters 2–4 (map, recipes, locking)
- [ ] examples ex01 (syscall), ex02 (map a page), ex03 (your stack growth), ex07 (F-key), ex11, ex12 (tutorials)

## 1 · Type U – user space only (the `atexit` kind)

| ✓ | Task | Time | What you practise |
|---|---|---|---|
| [ ] | A0-13 atexit | 20 min | the real A0 task of 2026: stub in `stdlib.c`, hook in `exit()` |
| [ ] | A0-14 string functions | 20 min | missing declarations, `strncpy` semantics |
| [ ] | A1-14 malloc | 30 min | a heap on a static pool, block headers |
| [ ] | A1-15 spinlock + mutex | 25 min | why a lock needs one atomic instruction |
| [ ] | A1-16 semaphore | 25 min | waiting without holding the lock |
| [ ] | A1-17 condition variables | 25 min | the lost wake-up |

## 2 · Type S – syscall + kernel state

| ✓ | Task | Time | What you practise |
|---|---|---|---|
| [ ] | A0-01 meminfo | 25 min | user struct, page-table walk, lock |
| [ ] | A0-02 allocpage | 30 min | `allocPPN` + `mapPage` / `unmapPage` + TLB |
| [ ] | A0-11 thread note | 30 min | user string, per-thread data (tutorial 1) |
| [ ] | A0-12 alias page | 40 min | two mappings of one frame (tutorial 2) |
| [ ] | A1-02 pthread_multiple | 45 min | all or nothing, `join_any` |
| [ ] | A1-03 barrier | 40 min | Mutex + Condition in the process |
| [ ] | A1-09 vtop / ptov | 25 min | page tables both ways |
| [ ] | A1-19 waitpid | 40 min | state that outlives a process |
| [ ] | A1-20 pthread_detach | 25 min | state that outlives a thread – or not |
| [ ] | A2-02 page backup | 40 min | the block device |

## 3 · Type E – the kernel reacts to an event

| ✓ | Event | Task | Time |
|---|---|---|---|
| [ ] | page fault | A0-03 heaplimit | 30 min |
| [ ] | page fault | A1-12 sbrk | 30 min |
| [ ] | page fault | A2-13 mmap | 45 min |
| [ ] | page fault + disk | A2-01 swap | 60 min |
| [ ] | page fault, copy-on-write | A2-09 zero page | 45 min |
| [ ] | loader | A0-05 read-only code | 25 min |
| [ ] | loader | A2-08 NX | 25 min |
| [ ] | timer | A1-07 thread runtime | 30 min |
| [ ] | timer | A1-18 sleep | 25 min |
| [ ] | scheduler | A1-04 thread flag | 30 min |
| [ ] | scheduler | A1-05 pthread_invoke | 30 min |
| [ ] | key | A2-06 F3 swap all | 30 min |
| [ ] | syscall entry | A2-12 syscall blacklist | 20 min |
| [ ] | kernel thread | A2-10 RAM obfuscation | 60 min |
| [ ] | boot | A2-11 boot counter | 40 min |

## 4 · Type A and K – registers

| ✓ | Task | Time | What you practise |
|---|---|---|---|
| [ ] | A1-13 sum15 | 15 min | arguments beyond the registers |
| [ ] | A1-11 page fault stats | 30 min | the interrupt entry, `cr2`, atomics |
| [ ] | A1-08 snapshot | 40 min | save and restore `user_registers_` + stack |
| [ ] | A0-07 upcall | 30 min | `rip`/`rsp`/`rdi`: the kernel calls user code |
| [ ] | A0-06 segv handler | 40 min | the same, on a page fault |
| [ ] | A0-09 pthread_multi | 40 min | a thread that runs a list of functions |

## 5 · Your team's own code – explain it

| ✓ | Task | What you practise |
|---|---|---|
| [ ] | A0-08 pthread_create | every step of your team's thread creation |
| [ ] | A1-01 pthread_join | your join – and the panic when the process exits (fix it in the team repo!) |
| [ ] | A1-06 pthread_cancel | your cancel – and the joiner that cannot be cancelled (fix it!) |

## 6 · The remaining tasks (when everything above is ticked)

A0-04 magic page (E), A1-10 shared page (S + E), A2-03 heap swap, A2-04 XOR swap, A2-05 dirty swap (E,
on top of A2-01), A2-07 mprotect (S + E). With these, every one of the 46 tasks is done once.

## 7 · The week before each interview

- [ ] three tasks you have not touched for a week: `task reset <id>`, timer, talk out loud
- [ ] one task of every type U, S, E, A/K – in the time box
- [ ] `task quiz 20`
- [ ] the cheat sheet ([a0/cheatsheet.pdf](a0/cheatsheet.pdf)) once more
- [ ] 15 minutes in the team repo itself: `git pull`, what changed since `team/BASE`?
