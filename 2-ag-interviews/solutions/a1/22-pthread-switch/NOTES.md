# A1-22 pthread_switch (context switch) – solution notes

Patch: [`solution.patch`](solution.patch) – libc only: `context.h`, `context.c`. Type A + U.

## The switch

```asm
context_switch:          # rdi = from, rsi = to
  pushq %rbp             # 1. save the callee-saved registers on the CURRENT stack
  pushq %rbx
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15
  movq %rsp, (%rdi)      # 2. from->rsp = rsp: the old context is complete now
  movq (%rsi), %rsp      # 3. rsp = to->rsp: we are on the other stack
  popq %r15              # 4. its callee-saved registers, in reverse order
  popq %r14
  popq %r13
  popq %r12
  popq %rbx
  popq %rbp
  ret                    # 5. pops ITS return address: continue where it stopped
```

- **Why only six registers:** for the compiler `context_switch` is an ordinary function call. The
  System V ABI says a call may destroy `rax rcx rdx rsi rdi r8–r11` – the caller saved what it
  still needs before the call. Only `rbx rbp r12–r15` (and `rsp`) must be the same afterwards.
- **Where `rip` is:** on the stack – `call context_switch` pushed the return address, `ret` pops
  it. One context = its `rsp`; everything else lies on its stack.
- **Order:** push and pop are mirror images. If they do not match, the other context gets wrong
  `rbx`/`rbp` – it crashes later, somewhere else (hard to debug: test with several coroutines).

## A context that never ran

```c
size_t top = (size_t) stack + stack_size;
top = top - top % 16;                      // aligned top
size_t* frame = (size_t*) top - 7;         // 7 values below the top, lowest = popped first
frame[0] = 0;                              // r15
frame[1] = 0;                              // r14
frame[2] = (size_t) arg;                   // r13
frame[3] = (size_t) function;              // r12
frame[4] = 0;                              // rbx
frame[5] = 0;                              // rbp
frame[6] = (size_t) __context_trampoline;  // popped by ret
ctx->rsp = (size_t) sp;
```
```asm
__context_trampoline:    # reached by ret, with r12 = function, r13 = arg
  movq %r13, %rdi
  callq *%r12            # function(arg)
  xorl %edi, %edi
  call exit              # it returned: there is nothing to return to
```

**Alignment:** after `ret`, `rsp` is the aligned top (a multiple of 16); the trampoline's `call`
pushes 8 → inside `function` `rsp % 16 == 8`, exactly what the ABI promises a function. Wrong by 8
and SSE instructions (`movaps`) on locals crash, or aligned locals are misaligned – the test
checks an `aligned(16)` buffer.

## Compared with the kernel (a tutor will ask)

| | this task | SWEB's kernel (timer, `yield`) |
|---|---|---|
| when | the code calls `context_switch` (cooperative) | any time a timer interrupt comes (preemptive) |
| what is saved | 6 callee-saved registers + `rsp` (+ `rip` on the stack) | ALL registers + flags (+ FPU), because the interrupted code did not expect it: `arch_interrupts.S` → `ArchThreadRegisters` |
| where | on the context's own stack | in `user_registers_` / `kernel_registers_` of the `Thread` |
| switch | `rsp = to->rsp; ret` | `currentThreadRegisters = ...; arch_contextSwitch()` (`iretq`) |

Making it preemptive would need a timer signal into user space (an upcall, A0-07) and saving
all registers – exactly what the kernel does.

## Not handled (fine for the task, mention it)

- No guard page: a coroutine that overflows its stack overwrites whatever lies below (here:
  another global).
- The x87 control word and `MXCSR` are callee-saved too, strictly speaking – irrelevant unless
  the program changes rounding modes.
