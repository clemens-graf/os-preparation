# A0-07 · upcall – register manipulation

**Time box 30 min · ★★☆ · read first: [ex05](../../examples/ex05-registers/)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §8 registers & calling convention, §9 diverting a thread

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         upcall
Nr:           1540 (sc_upcall)
Description:  upcall(function, argument) makes the calling thread execute
              function(argument) and then continue right after the upcall() call,
              which returns function's return value.
              Implement it ONLY by changing the user registers and the user stack in the
              kernel – no loop or call in libc.
Parameters:   size_t (*function)(size_t), size_t argument
Return Value: the return value of function, or -1 if function is not a user address
Notes:        upcalls may be nested (function may call upcall again).
```

## Test

`task start a0-07`, implement, `task test`. Done when all 6 checks pass.

## Level 2 – hints

<details><summary>1 – what would a "call" instruction do?</summary>

Push the address of the next instruction onto the stack, jump to the function. At the
moment of the syscall, the "next instruction" is the one after `int $0x80` – that is
`user_registers_->rip`. So: make room on the user stack, write `rip` there, point `rsp` at
it, set `rip = function`, `rdi = argument`.
</details>

<details><summary>2 – the red zone</summary>

Look at `objdump -d /tmp/sweb-ag/userspace/upcall_basic.sweb` → `<__syscall>`: it keeps
its local variables *below* `rsp` (`-0x38(%rbp)` while `rsp = rbp - 8`). That is allowed for
leaf functions – the 128 bytes below `rsp` (the red zone) belong to them. Writing your
return address at `rsp - 8` would destroy them. Skip the red zone first.
</details>

<details><summary>3 – where does the return value come from?</summary>

When `function` returns with `ret`, it jumps to the old `rip` with its result in `rax` –
exactly where `__syscall` expects the syscall result. Your syscall's own return value is
written into `rax` too, but before `function` runs, so it does not matter.
And the moved `rsp`? `__syscall` ends with `leave`, which restores `rsp` from `rbp`.
</details>

<details><summary>4 – a safe write to the user stack</summary>

The kernel may write to user memory directly, but if the page is not mapped, the write
page-faults inside the kernel. Check the slot with `checkAddressValid` (under the
page-table lock) and write through the returned kernel address.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Syscall plumbing for `upcall(function, argument)`.
2. Refuse a function address of 0 or `>= USER_BREAK`.
3. Take the calling thread's saved user registers (`currentThread->user_registers_`).
4. Compute the new stack pointer: old `rsp` minus 128 (red zone), round down to a multiple of 16, minus 8 (room for the return address).
5. Under `thread_lock_`: check that this stack address is mapped (`checkAddressValid`) – it gives you a kernel address for the slot; not mapped → return -1.
6. Write the **old `rip`** into that slot (the return address: the instruction after the syscall).
7. Set `rsp` to the new value, `rip` to the function, `rdi` to the argument. Return 0.
8. `task test` – the function runs and returns into the program as if it had been called.

</details>

## Your team's code

The task starts from your team's `main` (`osw26e3`, the commit in `team/BASE`) – the code you have in the interview.

- The registers are per thread: `currentThread->user_registers_`.
- The stack check: `process->getAddressSpace().checkAddressValid(rsp)` under
  `process->getThreadLock()` (`UserProcess* process = currentThread->getProcess();`).

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Draw the user stack right before <code>function</code> starts.</summary>

From high to low addresses: the old frame of `__syscall` (its locals in the **red zone**, 128 bytes below the old `rsp`), padding for alignment, then the **old `rip`** (the instruction after `int $0x80`) – `rsp` points at it, `rsp % 16 == 8`. Exactly like right after a real `call`.

</details>

<details><summary>Why does the kernel not need to save any other register?</summary>

`function` is entered like after a `call`: the ABI guarantees it keeps the callee-saved registers (`rbx`, `rbp`, `r12`–`r15`) and `rsp`. The caller-saved registers may be destroyed by any call anyway – `__syscall` doesn't rely on them after `int $0x80`, apart from `rax` (the result).

</details>

<details><summary>What happens with <code>rbx</code>, which <code>__syscall</code> uses as an input register?</summary>

`rbx` is callee-saved, so `function` restores it before `ret`. And `__syscall` only needed it as an *input* – after `int $0x80` its value doesn't matter anyway.

</details>

<details><summary>What would break if user code were compiled with <code>-fomit-frame-pointer</code>?</summary>

After the upcall, `__syscall` restores its `rsp` with `leave` (from `rbp`) – that repairs the moved `rsp`. Without a frame pointer it would do `add $n, %rsp` relative to the **moved** `rsp` and return to garbage. A robust kernel pushes a trampoline that restores the exact old `rsp` (Linux: `sigreturn`).

</details>
