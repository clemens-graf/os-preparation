# A0-06 segv handler – solution notes

Patch: [`solution.patch`](solution.patch) – for your team's code (`task solution`).

## Files touched

| File | What |
|---|---|
| `UserProcess.h/.cpp` | `segv_entry_`, `segv_handler_` (per process, under `thread_lock_`); `deliverSegv()` (takes `process_lock_`, then `thread_lock_`) |
| `PageFaultHandler.cpp` | before each `Syscall::exit(9999)` (both run after the locks are released): `if (user && getProcess()->deliverSegv(...)) return;`. Your team's `loadPage` already returns `false` for "no segment" (since 15 Nov) |
| `Syscall.h/.cpp`, `syscall-definitions.h` | `sc_setsegvhandler 1530`, `setSegvHandler(entry, handler)` on `currentThread->getProcess()` |
| `nonstd.h/.c` | `set_segv_handler()` + the static `segvEntry()` |

## The flow

```
user: *(int*)0xDEAD000 = 1
 → #PF → handlePageFault: valid → loadPage: no segment → false → is_exit, locks released
 → is_exit && user && getProcess()->deliverSegv(user_registers_, address)
      rsp' = rsp - 128, rounded down to a multiple of 16, minus 8   (red zone, alignment, return-address slot)
      slot mapped? (checkAddressValid)    no → false → kill as before
      *slot = 0; rsp = rsp'; rip = segv_entry_; rdi = address; rsi = segv_handler_
      segv_handler_ = 0                   (one-shot)
 → return → arch_contextSwitch loads user_registers_ → user runs
   segvEntry(address, handler) { handler(address); exit(139); }
```

## Key points

- **Two places kill on a segfault** – both `exit(9999)` in the page fault handler: the invalid
  branch (null, present+write, kernel address) and `is_exit` (no segment, guard page, no stack
  page). Base SWEB hides the second one in the loader (`exit(666)`); your team moved it out.
- **Only user-mode faults.** A fault in kernel mode (e.g. `write()` with a bad buffer) happened
  in the middle of kernel code: redirecting the user registers would abandon that kernel code
  half-way (locks held, state inconsistent).
- **Write the stack slot through the kernel address** that `checkAddressValid` returns: that
  write cannot page-fault. If the slot is not mapped – e.g. the fault *was* a stack overflow –
  there is no stack to run the handler on, and we kill as before (the real-world answer is
  `sigaltstack`, a separate signal stack).
- **libc entry function**: the kernel only ever jumps to `segvEntry`; it calls the user's
  handler and exits with 139 if it returns. 139 = 128 + 11 (SIGSEGV) – the Linux shell
  convention.

## Locking

`segv_entry_`/`segv_handler_` are written by the syscall and read + reset in the page
fault path of possibly another thread → same lock for both (your team's `thread_lock_`;
`checkAddressValid` reads the page tables, so `deliverSegv` takes `process_lock_` first). Read-check-reset in one critical section, otherwise two
threads faulting together could both get the one-shot handler.

## Variants the tutor could ask

- Also for CPU exceptions (division by zero): `errorHandler` in
  `arch/x86/64/source/InterruptUtils.cpp` (userspace branch, before `Syscall::exit(888)`).
- Handler gets the fault type too: pass the error flags in `rdx`.
- Resume after the handler (like a real signal handler): the kernel would push the old
  `rip`/`rsp` and need a `sigreturn` syscall – the handler cannot just return there.

## Shortest working version (tested)

[`short.patch`](short.patch) – **75 lines** instead of 87, passes the same test. Get it with
`task short a0-06` (branch `short/…`, then `task test`).

What is shorter: `deliverSegv` with fewer separate checks.

Use it to see the **core** of the task. In the interview, the full solution's extra checks are what
a tutor asks about ("what if the pointer is invalid / two threads at once?").
