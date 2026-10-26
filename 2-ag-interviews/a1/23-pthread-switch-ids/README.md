# A1-23 · pthread_switch: two threads swap their ids

**Time box 30 min · ★★☆ · type S (syscall + kernel) · on top of your team's join/detach**

**Theory first:** [concepts.pdf](../../concepts.pdf) §15 a thread's life · example: [ex01](../../examples/ex01-syscalls/)

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         pthread_switch (new, declare it in pthread.h)
Nr:           1730 (sc_pthread_switch)
Description:  The two threads a and b of the calling process exchange their ids.
              Afterwards the thread that was a is called b and the other way round:
              pthread_self() returns the new id, and pthread_join / pthread_cancel /
              pthread_detach with an id reach the thread that has it now.
              The main thread may take part too (also when it is the caller).
              Ended but not yet joined threads can be switched as well.
Parameters:   pthread_t a, pthread_t b
Return Value: 0, or -1 if a == b, one of them does not exist, or one of them is being
              joined right now
```

## Test

`task start a1-23` → `task test` (`switch_ids_basic.sweb`, then `exit`). Done when 9 checks pass.
Until `pthread_switch` is declared in `pthread.h`, the test does not compile – that is your first step.

## Level 2 – hints

<details><summary>1 – where does a thread's id live in your team's code?</summary>

In **two** places: in its record (`UserThreadData::tid_`, in `UserProcess::threads_`) and in the
thread object (`Thread::tid_`, read by `getTID()` – that is what `pthread_self` returns). Both
must change, or join (record) and pthread_self (object) disagree.
</details>

<details><summary>2 – which lock?</summary>

The process's `thread_lock_` – the same one join, detach and exit use for the records. Find both
records, check, and swap in **one** critical section.
</details>

<details><summary>3 – threads that already ended</summary>

An ended, not yet joined thread still has its record (with the exit value), but its `thread_`
is `nullptr`. Swap the id in the record anyway; only change the thread object if it still exists.
</details>

<details><summary>4 – why refuse while being joined?</summary>

A joiner sleeps on a record that it found by its old id. If you swap now, it would collect the
other thread's value – or the record it holds is renamed under its feet. Refusing is the simple,
safe rule.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. **Syscall plumbing (the six places, as in ex01):** a number 1730 in `syscall-definitions.h`, a
   declaration in `Syscall.h`, a `case` in `syscallException` that passes `arg1` and `arg2`, a
   function in `Syscall.cpp`, the declaration in the libc's `pthread.h`, and the wrapper in
   `pthread.c` that calls `__syscall` with the two ids.
2. **The syscall function** only finds the calling process and calls a new method of
   `UserProcess` with the two ids – the records are private to the process, so the work belongs
   there.
3. **Declare that method in `UserProcess.h`** (public), returning 0 or -1.
4. **In the method, take `thread_lock_` first** (a `ScopeLock` releases it at every return).
5. **Refuse `a == b`.**
6. **Find both records** with the process's lookup by tid. If one of them is missing, return -1.
7. **Refuse if either record is marked as being joined.**
8. **Swap the ids in the records:** the record of a gets b, the record of b gets a.
9. **Swap the ids in the thread objects:** for each record, if its thread object still exists,
   set that thread's id to the record's new id.
10. **Return 0.** Build, run the test, and check the four kinds of checks: return value,
    `pthread_self` of both, the joins, the error cases.

</details>

## Your team's code

`UserProcess::findThreadDataByTid` (private – so the method lives in `UserProcess`),
`UserThreadData{thread_, exit_value_, tid_, exited_, joined_, requested_cancel_, detached_}`,
`Thread::setTID` / `getTID`. Look at `UserProcess::detachThread` – it has exactly the structure you
need (lock, find, check, change).

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why do you change the id in two places?</summary>

The record (`UserThreadData::tid_`) is what join, cancel and detach search; the thread object's
id (`Thread::tid_`) is what `pthread_self` returns. If only one changes, `pthread_self` says one
thing and `pthread_join` finds another.

</details>

<details><summary>Why must the lookup, the checks and the swap be one critical section?</summary>

Otherwise a thread could end, be joined and its record deleted between "found it" and "swap" –
you would write into a freed record. Or two `pthread_switch` calls interleave and leave both
threads with the same id.

</details>

<details><summary>What happens to a cancel request or the detached flag when you swap?</summary>

They live in the record and the record moves with its thread – so they stay with the *thread*
(its work), only its name changes. A cancel that was requested for the work of a still ends that
work, now called b.

</details>

<details><summary>Could you swap "what they do" instead of the ids – the saved registers and stacks?</summary>

In principle yes (swap `user_registers_` and the stack bookkeeping), but it only works for threads
that are not inside the kernel right now – a thread sleeping in a syscall has kernel state that
cannot be exchanged. Swapping the ids gives the same visible result and is always safe.

</details>
