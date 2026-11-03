# A1-24 · pthread_invoke from a function pointer: make another thread run a function

**Time box 45 min · ★★★ · type K + S (the kernel sends another thread into user code) ([TRIAGE](../../TRIAGE.md)) · from the list: `pthread_invoke` – *"manually trigger other thread's execution from specific function pointer"***

**Theory first:** [concepts.pdf](../../concepts.pdf) §9 diverting a thread, §8 registers & calling convention · first: [A0-07 upcall](../../a0/07-upcall/) (the same trick for the *calling* thread)

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         pthread_invoke_function (new, declare it in pthread.h)
Nr:           1631 (sc_pthread_invoke_function), 1632 (sc_pthread_invoke_return)
Description:  pthread_invoke_function(thread, function, arg): the given thread of the
              calling process runs function(arg) the next time it leaves the kernel
              (returns from a syscall) and afterwards continues EXACTLY where it was -
              all its registers as before, also the result of the interrupted syscall.
              One pending call per thread at a time.
Parameters:   pthread_t thread, void (*function)(void*), void* arg
Return Value: 0, or -1 if thread is the caller / does not exist / has ended,
              function is NULL, or a call for that thread is still pending
```

## Test

`task start a1-24` → `task test` (`invoke_function_basic.sweb`, then `exit`). Done when 9 checks pass:
the function runs in the other thread with the argument, a second call works, the errors, and the
thread afterwards continues its loop with its local variable intact.

## Level 2 – hints

<details><summary>1 – you cannot change a running thread – so when?</summary>

The other thread's user registers (`user_registers_`) are saved whenever it is in the kernel. Do
not change them from the caller: only **store** the request in the target's `UserThread`
(function, arg). The target itself applies it at the **end of its next syscall**
(`Syscall::syscallException`, right before `return return_value;`) – then it is the current
thread, its registers are complete, and the result of its syscall is known.
</details>

<details><summary>2 – how to send a thread into a function (A0-07)</summary>

`rip = function`, `rdi = arg`, and a **return address** on its user stack: `rsp` below the red zone
(128 bytes), aligned so that `rsp % 16 == 8` at the function's first instruction, the return address
written at `rsp` (via `checkAddressValid`, under `thread_lock_`).
</details>

<details><summary>3 – how to come back with ALL registers</summary>

In A0-07 the function returns straight into the interrupted code – fine for the caller, which is
inside `__syscall`. Another thread could be anywhere, so save its registers (rip, rsp, rax, …, r15)
in its `UserThread` and let the function return into a **trampoline**: a tiny libc function that
makes a second syscall, `sc_pthread_invoke_return`, which copies the saved registers back. The
libc passes the trampoline's address as an extra syscall argument. (This is how signal handlers
come back in Linux: `sigreturn`.)
</details>

<details><summary>4 – the trap with copying ArchThreadRegisters</summary>

`ArchThreadRegisters` has a destructor that deletes its `fpu` buffer – a plain copy `saved = *regs`
would share that buffer and free it twice. Save the general registers one by one in your own small
struct.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Syscall numbers 1631 and 1632; two `case`s that call methods of the current process; the libc
   declaration in `pthread.h`.
2. In `UserThread`: fields for the pending call (function, arg, trampoline address), a flag "inside
   the invoked function", and a small struct for the saved registers.
3. `invokeFunction(tid, function, arg, trampoline)` in `UserProcess`: check the pointers (not 0, below
   `USER_BREAK`); under `thread_lock_` find the record by tid; refuse the caller, unknown or ended
   threads, and a thread with a call already pending; store the three values.
4. `deliverInvoke(return_value)` in `UserProcess`, called at the end of `syscallException` for every
   syscall: nothing pending, or already inside an invoked function → return. Otherwise compute the
   new `rsp` (red zone, alignment, room for the return address), write the trampoline address there,
   save all general registers (with `rax` = the syscall's result), set `rip`/`rdi`/`rsp`, clear the
   pending call, set the flag.
5. `invokeReturn()`: flag not set → -1. Copy the saved registers back into `user_registers_`, clear
   the flag, return the saved `rax` (the syscall handler writes the return value into `rax`).
6. libc: a C function that does the return syscall; a 2-line `__asm__` trampoline that `call`s it
   (the `call` fixes the stack alignment); `pthread_invoke_function` passes the trampoline's address
   as the 4th syscall argument.
7. `task test`.

</details>

## Your team's code

`UserThreadData::thread_` (nullptr once the thread ended), `UserProcess::findThreadDataByTid`
(under `thread_lock_`), `ArchMemory::checkAddressValid`, the syscall entry
`syscallHandler` in `arch/x86/64/source/InterruptUtils.cpp` (it writes the return value into
`user_registers_->rax` after `syscallException`). Arguments arrive in `rbx, rcx, rdx, rsi, rdi`
= `arg1 … arg5`.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why not change the target's registers directly in the caller's syscall?</summary>

Its saved registers are only complete while it is in the kernel, and if it is inside a syscall
right now, that syscall still has to write its result into `rax` – which would then be lost or
land in the wrong place. Letting the target apply the request itself, at its own syscall exit, has
neither problem.

</details>

<details><summary>What if the target never makes a syscall (a pure computing loop)?</summary>

Then it never runs the function – a limitation of this design. To fix it, also deliver at the end
of the timer interrupt when it returns to user mode (and then the saved registers are those of the
interrupted user code – exactly what the full save/restore is for).

</details>

<details><summary>Why the trampoline and a second syscall instead of returning directly?</summary>

Returning directly (like A0-07) only restores `rip`; the function may have changed `rax, rcx, rdx,
rsi, rdi, r8–r11`, which the interrupted code still needs. The second syscall restores every
register exactly – like `sigreturn` after a signal handler.

</details>

<details><summary>Why `rsp % 16 == 8` at the function and `call` in the trampoline?</summary>

The System V ABI expects `rsp % 16 == 8` at every function's first instruction (as if `call` had
just pushed the return address). After the function's `ret`, `rsp % 16 == 0` – so the trampoline
`call`s the C function, which pushes 8 bytes and restores the expected alignment.

</details>
