# Module 10 — Capstone: a thread library of your own

**Time:** 5–6 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 10)

Your P1 tasks are `pthread_create` and `pthread_join` in SWEB. Here you
build the same thing one level up, where a crash costs nothing: a pthreads
look-alike on top of `swapcontext`. Every problem of the real task shows
up: a fresh stack and a start routine; a return from the start routine
that must behave like `exit`; zombies holding return values; join errors
and cycles; a thread that cannot free the stack it runs on; blocking
instead of spinning; and, with the bonus, "interrupts" in the middle of
your library code.

## Steps

1. **Read** chapter 10 (thread control blocks and contexts, thread
   states, create/exit/join/detach semantics, reaping, user-level vs.
   kernel-level threads, blocking synchronisation, preemption and
   interrupt safety, how all of this maps onto SWEB).
2. **Examples** — `cd example && make run`:
   - `pingpong.c` — two contexts, two stacks, `swapcontext`, `uc_link`
   - `generator.c` — a coroutine that keeps its locals between calls
   - `switch_cost.c` — function call vs. user-level vs. kernel switch
3. **Assignment** — `assignment/uthread.c` (plumbing given, logic TODO):
   - Part A: `create`, `yield`, `exit`, `join` (with `ESRCH`, `EDEADLK`
     and cycle detection, `EINVAL`), `detach`, reaping, deadlock detection
   - Part B: blocking mutex with FIFO hand-off, condition variables
   - Part C (bonus): preemption by a CPU-time timer signal, and an
     interrupt-safe library
   ```bash
   cd assignment && make test     # or test-threads / test-sync / test-preempt
   ```
   A hanging test is stopped after 10 s with a hint. Tests that end the
   process (deadlock detection, the last thread exiting, stack overflow
   into the guard page) run in child processes.
4. **Paper questions** — `assignment/questions.md`. Q10 is a first draft
   of your P1 design: bring it to your team meeting.

## Done when

- `make test`: parts A and B pass (C passes too, with the bonus).
- You can explain where a SWEB pthread starts executing, what happens
  when its start routine returns, and who frees its stacks.

Reference solution and answers: `../solutions/10-uthreads/`.
