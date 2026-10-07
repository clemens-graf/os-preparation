# Module 05 — Condition variables and semaphores

**Time:** 4–5 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 5)

Locks answer "only one at a time". This module answers "wait until
something has happened" — without burning CPU and without losing
wake-ups. Part of P1's elective in SWEB is exactly this assignment:
user-space semaphores, demonstrated with readers–writers and the sleeping
barber.

## Steps

1. **Read** chapter 5 (waiting vs. spinning, condition variables and
   Mesa semantics, the while-loop rule, lost wake-ups, semaphores and their
   three roles, producer/consumer, readers–writers, sleeping barber,
   monitors).
2. **Examples** — `cd example && make run`:
   - `bounded_buffer.c` — THE condition-variable pattern; learn its shape
   - `lost_wakeup.c` — a wake-up that is lost when the check is not under
     the mutex, and the fixed version
   - `semaphores.c` — mutual exclusion, signalling and counting with
     POSIX semaphores
3. **Assignment** — `assignment/sync.c` (you may add struct fields in
   `sync.h`):
   - Part A: a counting semaphore from mutex + condition variable
   - Part B: a writer-preferring readers–writer lock
   - Part C: the sleeping barber, including a clean shutdown
   ```bash
   cd assignment && make test        # or make test-sem / test-rw / test-barber
   ```
4. **Paper questions** — `assignment/questions.md` (the last one is about
   `Scheduler::wake` in SWEB).

## Done when

- `make test` prints `==== all parts pass`.
- You can explain why every `pthread_cond_wait` sits in a `while` loop.

Reference solution and answers: `../solutions/05-condvars-semaphores/`.
