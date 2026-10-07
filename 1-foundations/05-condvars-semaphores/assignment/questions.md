# Module 05 — Paper questions

Answers: `../../solutions/05-condvars-semaphores/answers.md`.

**Q1.** Why must `pthread_cond_wait` be called in a `while` loop and not
an `if`? Give two different reasons.

**Q2.** What exactly does `pthread_cond_wait(&cv, &m)` do, step by step?
Which part must be atomic, and why?

**Q3.** When do you need `pthread_cond_broadcast` instead of
`pthread_cond_signal`? Give an example from this module's assignment.

**Q4.** What is the difference between a semaphore and a mutex? Could you
use a binary semaphore everywhere you use a mutex? What do you lose?

**Q5.** A semaphore "remembers" a post that happened while nobody was
waiting; a condition variable does not. Which of the two is therefore
immune to the lost wake-up of `example/lost_wakeup.c` by construction, and
what does the condition-variable version need instead?

**Q6.** Readers–writers: describe how *writers* can starve with a
reader-preferring lock, and how *readers* can starve with your
writer-preferring lock.

**Q7.** In the barber solution, why must the barber not hold the shop's
mutex while calling `cut_hair()`?

**Q8.** (SWEB) `Scheduler::wake(t)` loops `while (t->getState() != Sleeping)
yield();` before setting it to Running. Which race is it defending
against? What would happen without the loop?
