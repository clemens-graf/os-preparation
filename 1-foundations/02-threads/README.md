# Module 02 — Threads and the pthread API

**Time:** 3–4 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 2)

This is the API you will implement in SWEB as P1 (`pthread_create`, then
`pthread_join`). Here you *use* it on Linux, so you know exactly which
behaviour your SWEB version has to reproduce.

## Steps

1. **Read** chapter 2 (thread vs. process, what is shared, the thread
   lifecycle, create/join/exit/detach, argument and return-value passing,
   user-level vs. kernel-level threads).
2. **Examples** — `cd example && make run`:
   - `hello_threads.c` — create, pass per-thread arguments, join, collect
   - `shared_vs_private.c` — shared globals/heap vs. per-thread stacks
   - `return_values.c` — three ways to return results, `pthread_exit`
     from deep inside a call chain, detached threads
3. **Assignment**, `cd assignment && make test`:
   - **Part A** `parallel.c`: parallel sum / max-index / count-if with the
     *partition-then-combine* pattern — no locks allowed. Tests run under
     ThreadSanitizer *and* AddressSanitizer and also check that you create
     and join exactly `nthreads` threads.
   - **Part B** `bugs/`: three small programs, each with one classic
     threading bug. Fix each with a minimal change and write a one-line
     comment explaining the bug. Two of them often print the *correct*
     result and still fail — that is the point.

## Done when

- `make test` passes both parts (`7 passed` and `3 fixed`).
- You can explain why `pthread_create(&t, NULL, f, &i)` in a loop is wrong,
  and what `pthread_join` guarantees besides "waiting".

Reference solution: `../solutions/02-threads/`.
