# Module 06 — Deadlocks

**Time:** 3 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 6)

Fixing races (modules 03–05) means adding locks; adding locks creates the
next problem. With two teams in your group adding locks to shared SWEB
structures, lock ordering is something you will have to agree on.

## Steps

1. **Read** chapter 6 (Coffman conditions, resource-allocation / wait-for
   graphs, prevention vs. avoidance vs. detection, lock ordering, livelock,
   starvation, priority inversion, lock-order validators).
2. **Examples** — `cd example && make run`, then `make tsan`:
   - `deadlock_demo.c` — the two-lock deadlock, forced and fixed
   - `philosophers.c` — the naive dinner deadlocks, a watchdog detects it
   - `inversion.c` — a deadlock that does *not* happen, found anyway by
     ThreadSanitizer's lock-order graph
3. **Assignment** — `assignment/`:
   - Part A `accounts.c`: per-account locks — make transfers deadlock-free
   - Part B `dining.c`: dining philosophers, any correct strategy
   - Part C `lockdep.c`: your own lock-order validator that reports
     potential deadlocks from a single run
   ```bash
   cd assignment && make test     # or test-accounts / test-dining / test-lockdep
   ```
   The unfixed parts A and B *hang* — the tests detect that with a timeout.
4. **Paper questions** — `assignment/questions.md`.

## Done when

- `make test` prints `==== all parts pass`.
- You can explain which Coffman condition each of your fixes breaks.

Reference solution and answers: `../solutions/06-deadlocks/`.
