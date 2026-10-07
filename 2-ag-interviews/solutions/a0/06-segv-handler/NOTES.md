# A0-06 segv handler – solution notes

Patch: [`solution.patch`](solution.patch).

## Files touched

| File | What |
|---|---|
| `Loader.h/.cpp` | `loadPage` returns `bool` (false = no segment) instead of `exit(666)`; `segv_entry_`, `segv_handler_` (under `arch_memory_lock_`); `deliverSegv()` |
| `PageFaultHandler.cpp` | invalid / unloadable user fault → `deliverSegv`, only otherwise kill |
| `Syscall.h/.cpp`, `syscall-definitions.h` | `sc_setsegvhandler 1530`, `setSegvHandler(entry, handler)` |
| `nonstd.h/.c` | `set_segv_handler()` + the static `segvEntry()` |

## The flow

```
user: *(int*)0xDEAD000 = 1
 → #PF → handlePageFault: valid → loadPage: no segment → false
 → user && deliverSegv(user_registers_, address)
      rsp' = ((rsp - 128) & ~0xF) - 8     (red zone, alignment, return-address slot)
      slot mapped? (checkAddressValid)    no → false → kill as before
      *slot = 0; rsp = rsp'; rip = segv_entry_; rdi = address; rsi = segv_handler_
      segv_handler_ = 0                   (one-shot)
 → return → arch_contextSwitch loads user_registers_ → user runs
   segvEntry(address, handler) { handler(address); exit(139); }
```

## Key points

- **Two places kill on a segfault** – the page fault handler (invalid fault: null, present+
  write, kernel address) and the loader (no segment). Students usually find only the first.
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
fault path of possibly another thread → same lock for both (`arch_memory_lock_`, also
needed for `checkAddressValid`). Read-check-reset in one critical section, otherwise two
threads faulting together could both get the one-shot handler.

## Variants the tutor could ask

- Also for CPU exceptions (division by zero): `errorHandler` in
  `arch/x86/64/source/InterruptUtils.cpp` (userspace branch, before `Syscall::exit(888)`).
- Handler gets the fault type too: pass the error flags in `rdx`.
- Resume after the handler (like a real signal handler): the kernel would push the old
  `rip`/`rsp` and need a `sigreturn` syscall – the handler cannot just return there.
