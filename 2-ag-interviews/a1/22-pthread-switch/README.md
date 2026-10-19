# A1-22 · pthread_switch: the context switch itself

**Time box 40 min · ★★★ · type A + U (assembly, user space only)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §8 registers & calling convention, §9 diverting a thread · example: [ex05](../../examples/ex05-registers/), [ex14](../../examples/ex14-libc-function/)

From the list of earlier years: *"pthread_switch – manual context switching: asm-level logic that
saves thread A's registers and loads thread B's registers"*. The directed yield in the kernel is
A1-05; this task is the switch itself, written by you – in user space, where you can see every
register (like user-level threads / coroutines).

```
Name:         context_make, context_switch (new header context.h, libc only)
Description:  A context is a saved state of execution: registers + a stack.
              typedef struct { size_t rsp; } context_t;
              context_switch(from, to) saves the running context in *from and continues
              with *to - on the other stack, where that context stopped. It returns only
              when some code switches back to *from.
              context_make(ctx, stack, size, function, arg) prepares ctx so that the first
              switch to it calls function(arg) on that stack (16-byte aligned as the ABI
              wants, also if stack is not). If function returns, the process exits with 0.
Parameters:   context_t* from, context_t* to /
              context_t* ctx, void* stack, size_t stack_size, void (*function)(void*), void* arg
Return Value: - (both void)
```

## Test

`task start a1-22` → `task test` (`switch_basic.sweb`, `switch_return.sweb`, then `exit`) – until
`context.h` exists the tests do not even compile: that file is your first step. Done when
7 checks pass: two coroutines take turns (`ababab`), locals survive switches, each runs on its
own stack (deep recursion included), the stack is aligned, and a returning function ends the
process.

## Your team's code

Nothing in the kernel. A new header `userspace/libc/include/context.h` and a new
`userspace/libc/src/context.c` (the libc CMake globs `*.c`; `task test` re-runs cmake). Assembly in
a C file: a top-level `__asm__(".globl context_switch\n" "context_switch:\n" ...)` block defines a
function the C code can call.

## Hints

<details><summary>1 – which registers must be saved?</summary>

Only the **callee-saved** ones: `rbx`, `rbp`, `r12`–`r15` (System V ABI) – and `rsp`, and `rip`.
Everything else the *caller* of `context_switch` has already saved itself, because for the C
compiler it is just a function call. `rip` is free: the `call context_switch` pushed the return
address onto the stack.
</details>

<details><summary>2 – where to put them?</summary>

On the stack: push the six registers, store `rsp` in `from->rsp`. Then the whole context is
"`from->rsp` + what lies on that stack". Loading the other one is the mirror image: `rsp =
to->rsp`, pop the six registers, `ret` – which pops *that* context's return address.
</details>

<details><summary>3 – context_make: a context that never ran</summary>

Build the stack so it looks exactly as if `context_switch` had saved it: from the (aligned) top
downwards a return address, then six values for `rbp rbx r12 r13 r14 r15`; `ctx->rsp` points to the
last one. The return address is a small trampoline (also in `__asm__`) that calls `function(arg)` –
pass them in two of the restored registers, e.g. `r12` = function, `r13` = arg – and calls
`exit(0)` if it returns.
</details>

<details><summary>4 – alignment</summary>

At the first instruction of a function, `rsp % 16 == 8` (the `call` pushed 8 bytes onto an aligned
stack). So when the trampoline's `call` happens, `rsp` must be a multiple of 16: after the final
`ret` of `context_switch`, `rsp` must be exactly the aligned top.
</details>

## Questions a tutor might ask

- Why does `context_switch` not save `rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`–`r11`?
- Where is `rip` of the saved context?
- What is the difference to what the kernel does on a timer interrupt (A1-05, ex13)?
- What happens if the coroutine's stack is too small? Where would a guard page help?
- Why is this not preemptive – and what would you need to make it preemptive?
