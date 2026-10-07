# Module 10 — Paper questions

Answers: `../../solutions/10-uthreads/answers.md`.
Paths refer to your SWEB repo (`repos/osw26e3`). Q9 and Q10 are your P1
design in miniature — worth discussing with your team partner.

**Q1.** Why does every new uthread start in `trampoline` instead of in its
start routine? What would happen if `makecontext` pointed directly at the
start routine (with `uc_link = NULL`)? In SWEB, a user thread created by
`pthread_create` starts executing user code at some address the kernel
sets as its `rip`. What goes wrong when that function simply returns, and
how can libc's `pthread_create` wrapper prevent it?

**Q2.** Why can a detached thread not free its own stack in
`uthread_exit`? How does your library solve it? How does SWEB solve the
same problem for a kernel thread's stack (read `Thread::kill` and
`Scheduler::cleanupDeadThreads`)?

**Q3.** What is a zombie thread, and what must it keep? What happens to a
thread that is neither joined nor detached? What is the process-level
analogue (module 01)?

**Q4.** `uthread_join` detects cycles by following `joining` pointers from
the target. Why does this terminate, and why does it find every cycle
that the new join would close? Why does POSIX only say
`pthread_join` *may* return `EDEADLK`?

**Q5.** Your mutex hands ownership directly to the first waiter. Compare
with "unlock sets the mutex free and wakes a waiter, who then competes
again" (barging): what does each gain and lose? Which one does SWEB's
`Mutex::acquire` implement?

**Q6.** `uthread_cond_wait` must unlock the mutex and block
"atomically". Why is that automatic in parts A and B, what breaks with
preemption, and how does your part C make it atomic again? SWEB's
`Condition::wait` → `Lock::sleepAndRelease` unlocks the waiters list
*before* calling `Scheduler::sleep()` — why is that safe there?

**Q7.** Give a concrete interleaving that corrupts an unprotected
library under preemption (e.g. in `uthread_mutex_lock` or `q_push`).
Blocking SIGVTALRM corresponds to what in SWEB? Why would that not be
enough on a multi-core machine?

**Q8.** Why must the threads in the preemption tests not call `printf` or
`malloc`? Which rule in SWEB follows from the same reasoning?

**Q9.** Your uthreads are *M:1* (many user threads on one kernel thread),
SWEB's pthreads will be *1:1* (every pthread is a kernel `Thread`).
Compare switch cost (see `example/switch_cost`), what happens when one
thread blocks in `read()` on the keyboard, and the use of several cores.
What does each SWEB thread own that a uthread does not?

**Q10.** Map this module to your P1 tasks. For `pthread_create`,
`pthread_exit`, `pthread_join` and `pthread_detach` in SWEB, list what
you need: the system call and its argument checks (module 07), the new
thread's user stack (module 08), where it starts, what the kernel keeps
per thread for join, how the joiner waits (module 05), and who frees
what, when.
