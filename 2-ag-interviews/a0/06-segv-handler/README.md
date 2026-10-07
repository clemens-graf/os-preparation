# A0-06 · segv handler – call a user function on a segfault

**Time box 40 min · ★★★ · read first: [ex05](../../examples/ex05-registers/), [ex03](../../examples/ex03-stack-growth/)**

```
Name:         set_segv_handler
Nr:           1530 (sc_setsegvhandler)
Description:  The user registers a function handler(size_t address). When the process
              would be killed because of an invalid memory access in user mode
              (NULL pointer, address that no segment covers, write to a read-only page,
              kernel address), the kernel instead lets the faulting thread continue in
              handler(fault_address).
              The handler is one-shot: it is unregistered before it is called, so a
              fault inside the handler kills the process.
              If the handler returns, the process exits with exit code 139.
              set_segv_handler(NULL) unregisters the handler.
Parameters:   void (*handler)(size_t address)
Return Value: 0, or -1 if handler is not a user address
Notes:        Faults that happen in kernel mode (e.g. a bad pointer passed to write())
              do not call the handler.
```

## Test

`task start a0-06`, implement, `task test` (runs the three `segv_handler_*` programs).
Done when 5 checks pass, `segv_handler_return` exits with **139** and the other two are killed normally.

## Hints

<details><summary>1 – where are segfaults "handled" today?</summary>

Two places kill the process: `PageFaultHandler::handlePageFault` (invalid fault →
`Syscall::exit(9999)`) and `Loader::loadPage` (no segment → `Syscall::exit(666)`). The
second one is hidden – the fault looked valid to the page fault handler. Make `loadPage`
report failure (return `false`) and let the page fault handler decide.
</details>

<details><summary>2 – how does the thread "continue in the handler"?</summary>

`currentThread->user_registers_` are the registers the thread had when it faulted. After
the page fault handler returns, exactly these are loaded again. Set `rip` to the
function, `rdi` to its first argument – and give it a stack: below the red zone (128 bytes),
16-byte aligned, minus 8 for the return address (ex05).
</details>

<details><summary>3 – "if the handler returns, exit with 139"</summary>

The kernel cannot easily notice that a user function returned. Let libc do it: libc
registers its own entry function `entry(address, handler)` that calls `handler(address)`
and then `exit(139)`. The syscall gets both pointers; on a fault the kernel jumps to
`entry` with `rdi = address`, `rsi = handler`.
</details>

<details><summary>4 – what if the stack itself is broken?</summary>

If the fault was a stack overflow, there is no stack to run the handler on. Check that the
stack slot you want to write is mapped (`checkAddressValid` returns its kernel address)
and write through that kernel address. If not – kill as before.
</details>

<details><summary>5 – locking</summary>

The handler is per-process state, read in the page fault handler and written by the
syscall – one lock (the solution uses the page-table lock, since the stack check needs it
anyway). The one-shot reset must happen in the same critical section as the read.
</details>

## Questions a tutor might ask

- Why only for user-mode faults? What would go wrong for a fault inside `Syscall::write`?
- Why the 128 bytes? Why the `- 8`?
- What happens if the handler itself faults? Why one-shot?
- How would you also catch a division by zero? (`errorHandler` in `InterruptUtils.cpp`)
- How does Linux do this (signals, `sigreturn`, `sigaltstack`)?
