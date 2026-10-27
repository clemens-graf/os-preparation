# A0-06 · segv handler – call a user function on a segfault

**Time box 40 min · ★★★ · type K + E (page fault) ([TRIAGE](../../TRIAGE.md)) · read first: [ex05](../../examples/ex05-registers/), [ex03](../../examples/ex03-stack-growth/)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §9 diverting a thread, §4 page faults

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

## Your team's code

The task starts from your team's `main` (`osw26e3`, the commit in `team/BASE`) – the code you have in the interview.

- The registered handler is **per-process state → `UserProcess`**: `segv_entry_`, `segv_handler_`
  and `deliverSegv(registers, address)` (under `thread_lock_`, stack slot via
  `arch_memory_->checkAddressValid`).
- `Loader::loadPage(address, arch_memory)` returns `bool` (false = no segment) instead of
  `Syscall::exit(666)`.
- Your page fault handler: if `loadPage` succeeded → `return`; afterwards one common place for
  "invalid fault or no segment": `if (user && process->deliverSegv(currentThread->user_registers_,
  address))` → continue in the handler, else kill as before. (A guard-page hit stays a kill: the
  stack is broken, the handler would have none.)


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

The handler is per-process state (your `UserProcess`), read in the page fault handler and
written by the syscall – one lock (the solution uses the page-table lock, since the stack check needs it
anyway). The one-shot reset must happen in the same critical section as the read.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why only for user-mode faults? What would go wrong for a fault inside <code>Syscall::write</code>?</summary>

A user-mode fault interrupted user code – changing its registers is like a function call. A fault **inside the kernel** (e.g. `Syscall::write` touching a bad buffer) interrupted kernel code that may hold locks and be half-way through a change. Jumping to a user handler from there would abandon that kernel code with locks held and data inconsistent.

</details>

<details><summary>Why the 128 bytes? Why the <code>- 8</code>?</summary>

**128 bytes:** the red zone – the interrupted function may keep data below its `rsp` without moving it. **Alignment, then `- 8`:** the ABI says `rsp % 16 == 8` at the first instruction of a function (as if `call` had pushed a return address onto an aligned stack); the 8 bytes are that return-address slot.

</details>

<details><summary>What happens if the handler itself faults? Why one-shot?</summary>

The handler is **one-shot**: `segv_handler_` is reset before jumping. A fault in the handler then finds no handler and the process is killed – instead of an endless loop of faults and handler calls.

</details>

<details><summary>How would you also catch a division by zero? (<code>errorHandler</code> in <code>InterruptUtils.cpp</code>)</summary>

In `errorHandler` (`arch/x86/64/source/InterruptUtils.cpp`): in the userspace branch, before `Syscall::exit(888)`, call the same `deliverSegv`-like function (pass the vector number instead of an address).

</details>

<details><summary>How does Linux do this (signals, <code>sigreturn</code>, <code>sigaltstack</code>)?</summary>

The kernel delivers a signal by building a frame on the user stack (saved registers, `siginfo`) and jumping to the handler; when the handler returns it calls the `sigreturn` syscall, which restores the saved registers – the program continues. `sigaltstack` gives the handler a separate stack, so even a stack overflow can be handled.

</details>
