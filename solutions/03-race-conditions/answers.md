# Module 03 — Answers

**Q1.** A *data race*: two threads access the same memory location
concurrently, at least one access is a write, and nothing orders them
(no lock, no atomics, no join...). In C this is undefined behaviour. A
*race condition*: the program's correctness depends on the timing /
interleaving of threads. Example without a data race:
```c
if (atomic_load(&balance) >= 80)            /* atomic check */
  atomic_fetch_sub(&balance, 80);           /* atomic act   */
```
Each access is atomic, but check and act together are not: two threads
can both pass the check (overdraft).

**Q2.** (2+3)! / (2! 3!) = 10. For three threads with two instructions
each: 6! / (2! 2! 2!) = 90.

**Q3.** 1, 2 or 3.
- 3: the three increments do not overlap at all.
- 2: e.g. A and B overlap (both LOAD 0, both STORE 1), then C runs alone.
- 1: all three LOAD 0 before any of them STOREs.
It can never be 0: every thread stores the value it loaded plus one, and
the smallest value anyone can load is 0.

**Q4.** Yes. A timer interrupt can arrive between the LOAD and the STORE of
thread A. The scheduler switches to B, which does its complete `counter++`,
then later A resumes and STOREs its stale register value. Preemption on one
core produces exactly the same interleavings as parallelism on several
cores. (Only a single instruction such as `inc [counter]` cannot be
interrupted in the middle on one core — but you cannot rely on the compiler
emitting it, and it is not atomic on multicore without `lock`.)

**Q5.** `volatile` only forces the compiler to perform every load and store
in memory instead of caching the value in a register. The increment is
still three separate steps that other threads can interleave with. It gives
neither atomicity nor ordering guarantees between threads.

**Q6.** `pthread_join` synchronises: everything the joined thread did before
it terminated *happens-before* everything the joiner does after `join`
returns. The worker's write and main's read are therefore ordered — not
concurrent — so there is no data race.

**Q7.** An atomic per balance prevents lost updates on that one balance.
But a transfer changes *two* balances, and the invariant "the total stays
constant" spans all of them. Between the atomic decrement of `from` and the
atomic increment of `to`, a concurrent `bank_total` sees the money in
neither account. Invariants over several variables need one lock (or
transaction) covering all of them for the whole update and the whole read.

**Q8.** The check `config == NULL` still happens outside the lock:
several threads can pass it, then each takes the lock in turn and loads
the config again (and the unlocked read of `config` is a data race).
Fix: move the check inside the lock (or use `pthread_once`):
```c
pthread_mutex_lock(&m);
if (config == NULL)
  config = load_config();
pthread_mutex_unlock(&m);
```
(Keeping an unlocked "fast path" check in front is only correct with an
atomic flag and acquire/release ordering — see the bonus in `once.c`.)
