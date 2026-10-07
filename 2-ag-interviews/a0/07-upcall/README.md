# A0-07 · upcall – register manipulation

**Time box 30 min · ★★☆ · read first: [ex05](../../examples/ex05-registers/)**

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

## Hints

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

## Questions a tutor might ask

- Draw the user stack right before `function` starts.
- Why does the kernel not need to save any other register?
- What happens with `rbx`, which `__syscall` uses as an input register?
- What would break if user code were compiled with `-fomit-frame-pointer`?
