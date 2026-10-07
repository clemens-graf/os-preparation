# ex05 – change the user registers

`jump(function, argument)` does not return: the thread continues in `function(argument)`.
Test: `ex_registers.sweb` (3 checks).

## Why this works

When a user thread enters the kernel (syscall, page fault, interrupt), its registers are
saved in `currentThread->user_registers_` (`struct ArchThreadRegisters`:
`rip, rsp, rax, rdi, ...`). When the kernel returns to user space, `arch_contextSwitch()`
loads exactly these values again. Change them, and the thread continues elsewhere.

```cpp
ArchThreadRegisters* regs = currentThread->user_registers_;
size_t rsp = regs->rsp - 128;   // 1. skip the red zone
rsp &= ~0xFULL;                 // 2. 16-byte alignment ...
rsp -= sizeof(size_t);          // 3. ... minus 8: as if "call" had pushed a return address
*(size_t*) rsp = 0;             //    the return address: returning from function faults at 0
regs->rsp = rsp;
regs->rip = function;
regs->rdi = argument;           // System V ABI: arguments in rdi, rsi, rdx, rcx, r8, r9
```

1. **Red zone:** the 128 bytes below `rsp` belong to the interrupted function (leaf
   functions keep locals there – `__syscall` does!). Never write there.
2. **Alignment:** at function entry the ABI guarantees `rsp % 16 == 8`. Code using SSE
   (doubles, many memcpy) crashes with a #GP fault otherwise – the test uses a `double` for
   that reason.
3. The syscall's own return value is written to `rax` after your function returns – here it
   does not matter.

## Where this is used

A0-06 (segv handler), A0-07 (upcall), A1 snapshot/revive, signal-like tasks. In a page
fault the same registers are in `user_registers_` (the faulting thread's state).
