# A0-09 · pthread_multi

**Time box 40 min (with 08 done) · ★★★ · type S + K ([TRIAGE](../../TRIAGE.md)) · builds on [A0-08](../08-pthread-create/)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §15 a thread's life, §8 registers & calling convention

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         pthread_multi
Nr:           1552 (sc_pthread_multi)
Description:  Creates ONE new thread that calls functions[0](arguments[0]),
              functions[1](arguments[1]), ..., functions[count-1](arguments[count-1])
              one after the other, then ends.
              The caller may change or reuse both arrays immediately after the call
              returns.
Parameters:   pthread_t* thread, void* (**functions)(void*), void** arguments, size_t count
Return Value: 0, or -1 (count 0 or > 16, bad pointers, a function pointer that is not a
              user address)
Notes:        Start from your pthread_create (A0-08).
```

## Test

`task start a0-09` – this branch starts from the **A0-08 reference solution** (`base/a0-08`), so
you only add `pthread_multi`. If you prefer to build on your own A0-08, merge or cherry-pick your
`task/a0-08-pthread-create` branch into it first. Then `task test` (also re-runs `pthread_create_basic.sweb`).
Done when all 7 checks pass.

## Level 2 – hints

<details><summary>1 – who runs the loop?</summary>

The loop "call each function" can live in libc: a start function
`multiStart(functions, arguments, count)` that loops and then calls `pthread_exit`. The
kernel only has to start the thread there with three arguments (`rdi`, `rsi`, `rdx`).
</details>

<details><summary>2 – "may reuse the arrays immediately"</summary>

The new thread reads the arrays much later than the syscall returns. So the kernel copies
them at the time of the call – into kernel memory first (after checking every pointer), then
onto the **new thread's stack**, and passes the addresses of these copies.
</details>

<details><summary>3 – writing to another thread's stack</summary>

Before the new thread is added to the scheduler, nobody else can touch it – you may set up
its stack and registers freely. Write through the ident mapping (`checkAddressValid`
returns the kernel address of a user address in that address space). Keep the stack
16-byte aligned and leave 8 bytes for the "return address".
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Syscall `pthread_multi(id, functions, arguments, count, entry)`: check count (1–16) and every pointer.
2. Copy both arrays into kernel arrays **first**, then check every function pointer in the copy.
3. Create the thread with your team's `createThread(entry, false, …)` – not scheduled yet.
4. Push both arrays onto the **new thread's** stack (a helper in `UserThread` that copies through `checkAddressValid` under the lock and moves its `rsp` down, 16-byte aligned).
5. Set its `rdi` = address of the functions array, `rsi` = address of the arguments array, `rdx` = count.
6. Write the id to the user, then add the thread to the scheduler.
7. libc: an entry function that calls each function with its argument, then `pthread_exit(0)`. `task test`.

</details>

## Your team's code

The task starts from your team's `main` (`osw26e3`, the commit in `team/BASE`) – the code you have in the interview.

- Builds on **your team's `pthread_create`**: `process->createThread((void*) entry, false, 0, 0)`
  creates the thread with its own stack slot (`false`: no start_routine/arg registers), then you set
  `rdi`, `rsi`, `rdx` yourself before `Scheduler::instance()->addNewThread`.
- `UserThread::pushToStack()`: the new thread has only its top stack page mapped
  (`getUserStackLowestMappedVpn()`), enough for two arrays of 16 pointers; write through
  `getAddressSpace().checkAddressValid` under `getThreadLock()`.
- Number 1552 is free.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why copy the arrays, why not just pass the user pointers on?</summary>

The caller's arrays may live on its stack or be overwritten right after the syscall returns (the test does exactly that) – the new thread runs later, possibly much later. It needs its **own** copy that lives as long as it does.

</details>

<details><summary>Why copy into kernel memory first, and only then onto the new stack?</summary>

Check-then-use on **user** memory is a TOCTOU race: another thread of the process could change a function pointer after you checked it. Copy into the kernel first, check **the copy**, then put the checked copy onto the new thread's stack – that cannot be raced.

</details>

<details><summary>Could you do the whole thing in user space with <code>pthread_create</code>? What would be the difference?</summary>

Yes: `pthread_create(&t, 0, runAll, packed)` where `packed` holds a copy of both arrays – but that copy must live somewhere until the thread is done: on the heap (needs `malloc`, a stub in SWEB) or in static memory (then only one call at a time). The kernel version puts the copy on the new thread's own stack, which lives exactly as long as the thread.

</details>

<details><summary>What happens if one of the functions calls <code>pthread_exit</code>?</summary>

The thread ends right there – the remaining functions never run. That's the defined behaviour of `pthread_exit`: it ends the calling *thread*, whatever it was doing.

</details>
