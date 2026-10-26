# A1-21 · forkall

**Time box 45 min · ★★★ · type S (syscall + kernel) · on top of your team's `fork`**

**Theory first:** [concepts.pdf](../../concepts.pdf) §6 processes, §15 a thread's life · example: [ex06](../../examples/ex06-kernel-thread/)

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         forkall
Nr:           1720 (sc_forkall)
Description:  Like fork(), but the child process gets a copy of EVERY thread of the
              calling process, not only of the caller:
              - the caller's copy returns 0 (the caller itself gets the child's pid),
              - every other thread continues in the child exactly where it was (same
                registers - its stack is part of the copied address space),
              - a thread that was inside a syscall at that moment (e.g. sleeping in
                pthread_join) sees that syscall return -1 in the child,
              - the thread ids stay the same, so pthread_t values in memory stay valid:
                the child can join its copies with them.
Parameters:   -
Return Value: the child's pid in the parent, 0 in the child, -1 on failure
```

## Test

`task start a1-21` → `task test` (`forkall_basic.sweb`, then `exit` for the leak check). Done when
7 checks pass. The test runs a spinner thread (pure user mode) and a joiner thread (sleeping in
`pthread_join`), calls `forkall`, and checks in the child that the spinner keeps counting, that the
joiner's join returned -1, and that the child can join the spinner's copy with the old `pthread_t`.

## Level 2 – hints

<details><summary>1 – what has to be copied per thread?</summary>

The registers (the copy constructor does it), the **`UserThreadData`** (tid, exit value,
`exited_`, `detached_` – also for threads that ended but are not joined yet, so the child can
join them), the stack top and lowest mapped stack page of the `UserThread` (the copy constructor
does **not** copy them – see 4), and the bookkeeping `registerThread` does: `running_threads_++`,
`setRegisteredInProcess(true)`. Keep the tid: `setTID(data->tid_)`.
</details>

<details><summary>2 – which lock, and how long?</summary>

The parent's `thread_lock_` while you copy the address space and walk `threads_` – the list, the
stack slots and the page tables must not change underneath you. `loadExecutable` reads the disk:
do it **before** taking the lock. Add the copies to the scheduler at the end – while holding the
**child's** `thread_lock_`, so a copy that calls `exit()` at once cannot kill threads the scheduler
does not know yet.
</details>

<details><summary>3 – "inside a syscall" – how do you know?</summary>

A thread that is not running has its user registers saved – at the timer interrupt (then it was
in user mode, copy exactly) or at `int 0x80` (then it is inside a syscall; in the child there is
no kernel half of that syscall, so it must return -1: set the copy's `rax`). Mark it in
`UserThread`: a flag set at the top of `Syscall::syscallException` and cleared at its end.
(`switch_to_userspace_ == 0` is not enough: a page fault also enters the kernel, and its copy
must re-execute the faulting instruction with the original `rax`.)
</details>

<details><summary>4 – a bug you will find in your team's fork</summary>

`UserThread(child, parent)` leaves `user_stack_top_` and `lowest_mapped_stack_vpn_` at 0. In the
child the page fault handler then does not see a stack fault as one – a child that needs one more
stack page is killed with 666. Copy both (`setUserStackTop`, `setUserStackLowestMappedVpn`).
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Add a flag "inside a syscall" to `UserThread`; set it at the top of `Syscall::syscallException`, clear it before it returns.
2. Syscall plumbing for `forkall` (1720) and a method `UserProcess::forkall(caller)` next to `fork`.
3. Load the binary for the child **before** taking any lock (disk I/O).
4. Under the parent's `thread_lock_`: create the child process with a deep copy of the address space; copy the stack slot counter.
5. For every record of the parent: copy the record (same tid, value, flags; not "joined"). If its thread still runs: copy the thread with the copy constructor, give it the same tid, copy its stack top and lowest mapped stack page, set `rax` (caller: 0; a thread inside a syscall: -1; others: unchanged), register it in the child (counter, flag). Add the record to the child's list.
6. Release the parent's lock. Take the **child's** lock, start the child with the caller's copy, add every other copy to the scheduler, release.
7. Return the child's pid (the caller's copy gets 0 from its `rax`). `task test`.

</details>

## Your team's code

`UserProcess::fork(thread_to_clone_from)` (Felix): loads the binary again (`loadExecutable`),
creates the child with a **deep copy** of the address space (`new ArchMemory(*arch_memory_)` – all
stacks included), copies `next_stack_slot_`, makes ONE thread with the copy constructor
`UserThread(child, parent_thread)` (registers copied, `rax = 0`), `registerThread`, `startProcess`.
`Syscall::fork` turns 0 into -1. forkall is the same with a loop over `threads_`.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why can the copied threads simply continue – where is their state?</summary>

A thread that isn't running consists of its **saved user registers** (`user_registers_`, saved at the last timer interrupt or at `int 0x80`) plus its **stack**, which lies in the address space. The deep copy of the address space duplicates every stack; copying the registers (the `UserThread` copy constructor) duplicates the rest. In the child the copy resumes exactly where the original was.

</details>

<details><summary>What happens to a thread of the parent that sleeps in a lock or in <code>pthread_join</code> – in the parent, and in the child?</summary>

**In the parent** nothing changes – it keeps sleeping in its lock or join. **In the child** its copy has no kernel half (the parent's kernel stack, the Condition it sleeps on, a held lock are not copied): it continues in user mode right after `int 0x80`, so its syscall must return something – -1, "failed" (`in_syscall_` decides). Locks held in *user space* (A1-15) are copied as "held" – a classic forkall problem.

</details>

<details><summary>Other threads of the parent keep running while you copy the address space. Is the copy consistent? How would you make it consistent?</summary>

No: while the parent's lock is held, its other threads can still run in user mode and write memory without any lock – a thread may be copied with registers from one moment and stack/heap contents from a slightly later one. Consistent: stop all other threads first (a "please stop" flag checked at every syscall and timer tick, wait until all are stopped), then copy, then let them go.

</details>

<details><summary>Why do you add the copies to the scheduler only at the end, under the child's lock?</summary>

The caller's copy could run immediately and call `exit()`; `exitProcess` kills every thread in the child's list – one that isn't known to the scheduler yet would never be cleaned up (and the process never deleted). Holding the **child's** `thread_lock_` makes `exitProcess` (which takes it) wait until all copies are scheduled.

</details>

<details><summary>Why keep the thread ids – and what would break if the child got new ones?</summary>

`pthread_t` values are stored in **user memory** (globals, the stack) – e.g. the test's `spinner_tid`. If the copies got new tids, every stored id would be wrong in the child: `pthread_join`/`pthread_cancel` with them would fail or hit the wrong thread. Same tids are fine, because tids only need to be unique **within a process** (`findThreadDataByTid` searches one process).

</details>
