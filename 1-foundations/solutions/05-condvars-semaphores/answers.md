# Module 05 — Answers

**Q1.** (a) Spurious wake-ups: POSIX allows `pthread_cond_wait` to return
without any signal. (b) Stolen wake-ups (Mesa semantics): between the
signal and the moment the woken thread re-acquires the mutex, another
thread can grab the mutex and consume the state (e.g. take the last item).
(c) With broadcast, several threads wake but only some can proceed. In all
cases the condition must be re-checked after waking.

**Q2.** (1) Put the calling thread on the condition variable's wait queue,
(2) release the mutex, (3) sleep; when woken: (4) re-acquire the mutex,
(5) return. Steps 1–3 must be atomic with respect to other threads using
the mutex: if the mutex were released before the thread is registered as a
waiter, a signal could arrive in between and be lost.

**Q3.** When more than one waiter may be able to proceed (e.g. a writer
releases the rwlock: *all* waiting readers may enter → broadcast
`readers_ok`), or when waiters on the same condition variable wait for
*different* things (the barber's `haircut_done`: only the customer whose
ticket finished can go; `signal` might wake a different customer, who
goes back to sleep, and the right one never wakes).

**Q4.** A mutex has an owner: only the thread that locked it may unlock it
(enforceable, enables deadlock detection, priority inheritance, "who holds
this?" debugging — SWEB's `Lock` tracks `held_by_`). A semaphore is just a
counter; any thread may post. A binary semaphore works as a mutex
functionally, but you lose ownership checks: unlocking someone else's lock
or double-unlocking (value becomes 2 → two threads inside) go unnoticed.

**Q5.** The semaphore: a post increments the counter even with no
waiters, and a later wait just decrements it without sleeping. The
condition variable needs the *state* stored separately (a flag/counter
under the mutex) and the rule "check the state and wait while holding the
mutex".

**Q6.** Reader preference: as long as at least one reader is inside, new
readers may enter; with overlapping readers `active_readers` never drops
to 0 and the writer waits forever. Writer preference: while writers keep
arriving (`waiting_writers > 0`), no reader is admitted, so a steady stream
of writers starves the readers. Fair solutions alternate (e.g. phase-fair
or FIFO queue).

**Q7.** Holding the mutex during the haircut would block every customer who
arrives (they need the mutex to check for a free chair) for the whole
haircut — the shop would be serialised. Worse, calling unknown code while
holding a lock risks deadlock if that code takes other locks. Rule: hold a
lock only for the short state update, never across slow or foreign code.

**Q8.** A lost wake-up: thread T has decided to sleep (e.g. put itself on
a lock's waiter list) but has not yet set its state to `Sleeping` and
yielded. If another thread calls `wake(T)` in this window, it sets T to
`Running` — then T executes `setState(Sleeping)` and sleeps with nobody left
to wake it. The loop makes the waker wait until T has actually gone to
sleep, so the wake cannot overtake the sleep.
