# A0-07 upcall – solution notes

Patch: [`solution.patch`](solution.patch).

## The code

```cpp
size_t Syscall::upcall(size_t function, size_t argument)
{
  if (function == 0 || function >= USER_BREAK)
    return (size_t) -1;

  ArchThreadRegisters* regs = currentThread->user_registers_;
  size_t rsp = ((regs->rsp - 128) & ~0xFULL) - sizeof(size_t);

  Loader* loader = currentThread->loader_;
  {
    ScopeLock lock(loader->arch_memory_lock_);
    pointer slot = loader->arch_memory_.checkAddressValid(rsp);
    if (!slot)
      return (size_t) -1;
    *(size_t*) slot = regs->rip;     // the return address: instruction after int $0x80
  }
  regs->rsp = rsp;
  regs->rip = function;
  regs->rdi = argument;
  return 0;                          // lands in rax, overwritten by function's result
}
```

## Why it works – the user stack right before `function` runs

```
higher addresses
  ...                     <- old rsp (inside __syscall)
  red zone (128 bytes)       __syscall's locals live here (it is a leaf function!)
  padding (alignment)
  [old rip]               <- new rsp   (rsp % 16 == 8, as after a real call)
lower addresses
```

`function` returns with `ret` → pops the old `rip` → continues in `__syscall` right after
`int $0x80` with `rax` = function's result. `__syscall` (compiled with `-O0`) then does
`mov %rax,-0x10(%rbp)`, `leave`, `ret`: `leave` restores `rsp` from `rbp`, so our moved
`rsp` is repaired. Callee-saved registers (`rbx`, `rbp`, `r12`–`r15`) are preserved by
`function` because it follows the ABI.

## Pitfalls

- **Forgetting the red zone**: writing the return address at `rsp - 8` overwrites
  `__syscall`'s locals (`objdump -d` shows them at `-0x10(%rbp)` ... `-0x38(%rbp)` while
  `rsp = rbp - 8`). In this case they happen to be dead after the syscall – but in general
  that is luck.
- **Misaligned stack**: functions that use SSE (`printf` with floating point, many
  compilers' memcpy) crash with #GP if `rsp % 16 != 8` at entry.
- Writing the user stack directly instead of via the ident address: works as long as the
  page is mapped – if not, you page-fault in the kernel while holding the lock.

## Answers to the tutor questions

- *Other registers?* Only what a `call` changes: `rsp` (push), `rip`, plus the argument in
  `rdi`. Everything else is the callee's/ABI's business.
- *`-fomit-frame-pointer`:* then `__syscall` would restore `rsp` by `add $n, %rsp` relative
  to the *moved* `rsp` → garbage. A robust kernel would push a trampoline that restores
  the exact old `rsp` (like Linux's `sigreturn`).
