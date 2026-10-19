# Training plan – everything once, every type covered

If you work through this list, you have practised **every kind of task** the AG interviews can
throw at you – not only syscalls. Every task runs on your team's code (`task start <id>`).
Tick a line when the task passes **without looking at the solution**, in its time box, and you
can answer the card's tutor questions out loud.

Before each task: read the **theory section** its card names ([concepts.pdf](concepts.pdf) – what a heap
is, what `exit` does, what a register is for …; 10–15 min the first time). Then **30 seconds of triage** – type (U / S / E / A / K, [TRIAGE.md](TRIAGE.md)) and
the first file ([ATLAS.md](ATLAS.md)). Say both out loud. Then start the timer.

## 14-day plan: A1 + the A0 tasks that matter for A1 (19 Oct – 1 Nov 2026)

23 tasks: all 17 A1 tasks up to A1-19 that are not study tasks (A1-01, A1-06, A1-20 are your own,
merged code), plus the 6 A0 tasks whose content comes back in A1:
A0-03 (page fault handler → A1-12), A0-07 (registers → A1-08, A1-13), A0-09 (threads on your team's
`createThread`), A0-11 (per-thread state), A0-13 `atexit` and A0-14 (libc-only – like A1-14 … A1-17;
`atexit` was the real A0 task – your team's `main` has your version now: replace it by a stub on
the task branch and write it again, see the card).
Not needed for A1: A0-04, A0-05 (A2 material), A0-12 (A1-10 covers it); A0-06 only if time is left.

About **1–1.5 h a day**:

1. **Redo (≤ 20 min):** every task from 2 days ago that you only finished with hints or the
   solution: `task reset <id>`, again from scratch, without looking.
2. **The day's tasks**, each in its time box. Stuck after the time box: one hint, 10 more minutes,
   then `task diff <id>` + `NOTES.md` – every line out loud with its "why". Mark it "needs redo".
3. **Anki 15 min** (due cards) + `task quiz 5`.

| ✓ | Day | Date | Tasks (time box) | Type | Note |
|---|---|---|---|---|---|
| [ ] | 1 | Mon 19 Oct | A1-09 vtop/ptov (25) · A0-14 strings (20) · A1-14 malloc (30) | S · U · U | warm-up, three different places |
| [ ] | 2 | Tue 20 Oct | A0-03 heaplimit (30) · A1-12 sbrk (30) | S+E · S+E | the same pattern twice: page fault handler |
| [ ] | 3 | Wed 21 Oct | A1-18 sleep (25) · A0-07 upcall (30) | S+E · K | timer; registers |
| [ ] | 4 | Thu 22 Oct | A1-13 sum15 (15) · A0-13 atexit (20) · A1-15 spinlock + mutex (25) | A · U · U | short – debate week: read your crib sheet |
| [ ] | 5 | Fri 23 Oct | A1-19 waitpid (40) · A1-16 semaphore (25) | S · U | processes; builds on A1-15 |
| [ ] | 6 | Sat 24 Oct | A1-04 thread flag (30) · A1-17 cond (25) | E+S · U | scheduler; builds on A1-15 |
| [ ] | 7 | Sun 25 Oct | A0-11 thread note – finish (30) · A1-05 pthread_invoke (30) | S · E+S | PoC tag day – keep it light |
| [ ] | 8 | Mon 26 Oct | A1-02 pthread_multiple (45) | S | threads: all or nothing |
| [ ] | 9 | Tue 27 Oct | A1-08 snapshot (40) · A1-07 thread runtime (30) | A+S · E+S | registers (after A0-07); timer |
| [ ] | 10 | Wed 28 Oct | A1-11 page fault stats (30) | A+E | interrupt entry – plus redo time |
| [ ] | 11 | Thu 29 Oct | A1-03 barrier (40) | S | Mutex + Condition |
| [ ] | 12 | Fri 30 Oct | A0-09 pthread_multi (40) | S+K | PoC deadline – only one task |
| [ ] | 13 | Sat 31 Oct | A1-10 shared page (45) | S+E | the longest one last |
| [ ] | 14 | Sun 1 Nov | **redo day:** every task still marked "needs redo" · `task quiz 20` | | team meeting 2 (walkthrough) |

Time left on days 8–13? Optional: A0-06 segv handler (40, K + page fault).

After day 14 – the two new tasks from the list of earlier years, then section 7:

| ✓ | When | Task (time box) | Type |
|---|---|---|---|
| [ ] | Mon 2 Nov | A1-22 pthread_switch: the context switch itself (40) | A + U |
| [ ] | Tue 3 Nov | A1-21 forkall (45) | S |

Then 2–15 Nov: a second round of the hardest tasks without hints, 2–3 mock interviews, and the
week before the AG (16–20 Nov) – section 7.

## 0 · Orientation (once, ~2 h)

- [ ] [TRIAGE.md](TRIAGE.md) and [ATLAS.md](ATLAS.md) sections 1–2 read
- [ ] `task quiz 20` – twice, on different days (47 questions: places in the code + "which type?")
- [ ] the Anki deck imported ([anki/](anki/README.md)) – then a few minutes every day until the interview
- [ ] guide chapters 2–4 (map, recipes, locking)
- [ ] examples ex01 (syscall), ex02 (map a page), ex03 (your stack growth), ex07 (F-key), ex11, ex12 (tutorials), ex13 (timer + scheduler – before day 3), ex14 (libc only – before day 1)

## 1 · Type U – user space only (the `atexit` kind)

| ✓ | Task | Time | What you practise |
|---|---|---|---|
| [ ] | A0-13 atexit | 20 min | the real A0 task of 2026: your team has it now – write it again from a stub |
| [ ] | A1-22 pthread_switch | 40 min | the context switch in assembly: callee-saved registers, `rsp`, `ret` |
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
| [ ] | A1-21 forkall | 45 min | copy every thread: registers + stack + record |
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
| [ ] | A1-06 pthread_cancel | your cancel – the cancellation point in `pthread_join` you fixed on 18 Oct |
| [ ] | A0-13 atexit | your `atexit` – explain it, then write it again from a stub |
| [ ] | A1-20 pthread_detach | your detach – explain it, then write it again from a stub |

## 6 · The remaining tasks (when everything above is ticked)

A0-04 magic page (E), A1-10 shared page (S + E), A2-03 heap swap, A2-04 XOR swap, A2-05 dirty swap (E,
on top of A2-01), A2-07 mprotect (S + E). With these, every one of the 48 tasks is done once.

## 7 · The week before each interview

- [ ] three tasks you have not touched for a week: `task reset <id>`, timer, talk out loud
- [ ] one task of every type U, S, E, A/K – in the time box
- [ ] `task quiz 20`, the Anki deck due cards
- [ ] the cheat sheet ([a0/cheatsheet.pdf](a0/cheatsheet.pdf)) once more
- [ ] 15 minutes in the team repo itself: `git pull`, what changed since `team/BASE`? (pinned:
  `f78faff3`, 19 Oct – if a lot changed, ask Claude to re-pin)
