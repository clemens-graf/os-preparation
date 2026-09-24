# Module 04 — Locks from the hardware up

**Time:** 3–4 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 4)

How do you *build* a lock? Only from atomic hardware instructions — plain
loads and stores are not enough on a real CPU (the Peterson demo proves
it). This is also exactly how SWEB's `SpinLock` and `Mutex` work, and
`pthread_spin_*` in SWEB's libc is one of your P1 elective tasks.

## Steps

1. **Read** chapter 4 (requirements for mutual exclusion, disabling
   interrupts, Peterson and why it breaks, test-and-set, CAS, fetch-and-add,
   spin vs. sleep, memory ordering, SWEB's locks).
2. **Examples** — `cd example && make run`, then `make asm`:
   - `peterson.c` — correct on paper, broken on your CPU, fixed by fences
   - `spinlock_tas.c` — test-and-set lock; pure spinning vs. yield vs. mutex
     with more threads than cores (look at the CPU time!)
   - `cas.c` — compare-and-swap loops: atomic max and a CAS lock
3. **Assignment** — `assignment/locks.c`:
   - Part A: test-and-test-and-set spinlock with `sched_yield` + trylock
   - Part B: ticket lock (fair, FIFO)
   - Part C (bonus): a futex-based mutex whose waiters sleep in the kernel
   ```bash
   cd assignment && make test
   ```
4. **Paper questions** — `assignment/questions.md`.

## Done when

- `make test`: 5 passed (7 with the bonus), both optimised and under TSan.
- You can explain why SWEB's `SpinLock` yields instead of spinning.

Reference solution and answers: `../solutions/04-locks/`.
