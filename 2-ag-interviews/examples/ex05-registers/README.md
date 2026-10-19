# ex05 – change the user registers

`jump(function, argument)` does not return: the thread continues in `function(argument)`.
Test: `ex_registers.sweb` (3 checks).

## What the example changes

| File | Lines |
|---|---|
| `common/include/kernel/Syscall.h` | +3 |
| `common/include/kernel/syscall-definitions.h` | +3 |
| `common/source/kernel/Syscall.cpp` | +25 |
| `userspace/libc/include/nonstd.h` | +5 |
| `userspace/libc/src/nonstd.c` | +5 |

The code below is the important part. The exact, complete change: `example.patch` in this folder, or `task example ex05` and then
`git -C ~/Documents/University/2026-fall/os/repos/sweb-ag show` (`q` quits).

## Why this works

When a user thread enters the kernel (syscall, page fault, interrupt), its registers are
saved in `currentThread->user_registers_` (`struct ArchThreadRegisters`:
`rip, rsp, rax, rdi, ...`). When the kernel returns to user space, `arch_contextSwitch()`
loads exactly these values again. Change them, and the thread continues elsewhere.

The whole function from `example.patch` (`common/source/kernel/Syscall.cpp`):

```cpp
size_t Syscall::jump(size_t function, size_t argument)
{
  if (function >= USER_BREAK)
    return (size_t) -1;

  ArchThreadRegisters* regs = currentThread->user_registers_;
  size_t rsp = regs->rsp - 128;   // leave the red zone alone: leaf functions keep locals below rsp
  rsp = rsp - rsp % 16;           // System V ABI: rsp is 16-byte aligned before a call ...
  rsp -= sizeof(size_t);          // ... and the call pushes an 8 byte return address
  *(size_t*) rsp = 0;             // that return address: if function returns, it faults at 0

  regs->rsp = rsp;
  regs->rip = function;
  regs->rdi = argument;           // first argument register of the System V ABI
  return 0;                       // ends up in rax - the new function ignores it
}
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
