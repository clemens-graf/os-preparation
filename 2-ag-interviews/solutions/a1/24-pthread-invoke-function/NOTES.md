# A1-24 pthread_invoke from a function pointer – solution notes

Patch: [`solution.patch`](solution.patch) – on your team's code (`4778e63b`). Tested: 9 of 9.

## The three parts

```cpp
// 1. the caller only stores the request (under thread_lock_)
size_t UserProcess::invokeFunction(size_t tid, size_t function, size_t arg, size_t trampoline)
{ ...checks...  thread->invoke_function_ = function; thread->invoke_arg_ = arg;
                thread->invoke_trampoline_ = trampoline; return 0; }

// 2. the TARGET applies it at the end of its next syscall (end of syscallException)
void UserProcess::deliverInvoke(size_t return_value)
{
  if (nothing pending || already inside an invoked function) return;
  rsp = user rsp - 128 (red zone), aligned down to 16, minus 8 (room for the return address)
  write the trampoline address at rsp (checkAddressValid)
  save rip, rflags, rax = return_value, rcx ... r15
  rsp = new rsp; rip = function; rdi = arg; pending = 0; running = true;
}

// 3. the trampoline's syscall restores everything
size_t UserProcess::invokeReturn()
{ copy the saved registers back; running = false; return saved rax; }
```

libc: `__pthread_invoke_trampoline` (asm: `call __pthread_invoke_finish`) → `__syscall(sc_pthread_invoke_return)`.

## Why each step

- **Deliver at the target's own syscall exit:** its registers are complete and its syscall result is
  known – changing them from the caller would race with that result.
- **Save all general registers, restore with a second syscall:** the function may clobber the
  caller-saved registers that the interrupted code still uses (the `sigreturn` idea).
- **Field by field, not `saved = *regs`:** `ArchThreadRegisters` deletes its `fpu` buffer in the
  destructor – a struct copy would free it twice. (The FPU state is not saved: do not use floating
  point in the invoked function.)
- **Red zone + alignment:** as in A0-07.
- **One pending call, none while running:** a nested delivery would overwrite the saved registers.

## Limitation

A thread that never makes a syscall never runs the function. Variant: also deliver when the timer
interrupt returns to user mode.

## Shortest working version (tested)

[`short.patch`](short.patch) – **66 lines** instead of 107, passes the same test. Get it with
`task short a1-24` (branch `short/…`, then `task test`).

What is shorter: registers saved with one `memcpy` (first 19 words); the delivery written directly at the end of `syscallException`; no check that the return syscall comes from an invoked function.

Use it to see the **core** of the task. In the interview, the full solution's extra checks are what
a tutor asks about ("what if the pointer is invalid / two threads at once?").
