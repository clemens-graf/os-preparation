# Module 07 — Answers

**Q1.** Two hardware barriers. (1) Kernel pages are mapped *without*
the user bit in the page tables: fetching an instruction there in ring 3
raises a page fault, and SWEB's `checkPageFaultIsValid` ("You are
accessing a kernel address in user-mode") ends the process. (2) The only
way to enter ring 0 is through the gates the kernel set up: an interrupt
gate lists the lowest privilege level that may use it with `int n`
(the DPL). SWEB gives DPL 3 to vector 0x80 only
(`interrupt_gates[i].dpl = (i == SYSCALL_INTERRUPT ...) ? DPL_USER_SPACE :
DPL_KERNEL_SPACE`); `int $0x0e` from user mode raises a
general-protection fault instead. The gate also fixes the *entry
address*: the user picks the syscall number, never the code that runs.

**Q2.** `buffer = 0x1000`, `size = 0xfffffffffffff800`:
`buffer + size` wraps around to `0x800`, which is `<= USER_BREAK`, so
the check passes. (Also `buffer = USER_BREAK - 16`, `size = SIZE_MAX`,
which wraps to `USER_BREAK - 17`.) In the fd-1 branch, `(int)size` is
-2048. With standard printf semantics a negative precision counts as
"no precision", so `%.*s` prints up to the next NUL byte, wherever that
is. For a file, `VfsSyscall::write` would try to copy the whole huge
range. On 32-bit SWEB, whose kernel begins right at `USER_BREAK`, it runs
straight into kernel memory. Correct, and unable to overflow:
```c
if (size > USER_BREAK || buffer > USER_BREAK - size) return -EFAULT;
```
Rule: never add two numbers the user controls; subtract from a constant instead.

**Q3.** Only the first byte is checked. A path that starts just below
`USER_BREAK` and has no NUL there continues into kernel memory. On
32-bit that memory is the kernel itself, so kernel bytes become part of
the file name. On 64-bit it is the non-canonical hole, and the kernel
faults. `strlen(path)` cannot be the fix, because `strlen` *is* the
unchecked read: it walks the memory before anything was validated, and
its answer can be stale a moment later (Q4). The fix is to copy the
string into a bounded kernel buffer, checking every page as the copy
reaches it (`strncpy_from_user`), and then use only the copy.

**Q4.** Thread A calls `writev` with `iov[0] = {good_buf, 64}`. Thread B
keeps flipping `iov[0].base` between `good_buf` and a kernel address.
Sooner or later B's store lands between the kernel's *check* (first
read of `iov[i].base`) and its *use* (second read), and the kernel
prints 64 bytes of kernel memory. This is a **TOCTOU** (time of check to
time of use) race. A kernel that reads the same user value twice is said
to have a **double fetch**. Fix: copy the iovec array into kernel memory
*once* (`copy_from_user`), then check and use only that copy. It matters
from A1 on because `pthread_create` makes multi-threaded processes that
share one address space. SWEB enables interrupts during system calls
(`syscallHandler`), so even on one CPU the timer can switch to thread B
in the middle of thread A's system call.

**Q5.** (a) A valid result can have its top bit set. On 32-bit, `mmap`
can return an address above 2 GiB, which is negative as a `long`. Linux
reserves exactly the values -4095..-1 (`MAX_ERRNO`) for errors, and
every other value is a result. (b) Thread 1 fails with `EBADF`, thread 2
fails with `ENOENT` in between, and thread 1 then reads `ENOENT`. A
global `errno` is also a plain data race (module 03). SWEB's libc has no
`errno` yet. Any per-thread libc state you add after A1 (`errno`,
`strtok`'s position, ...) must live in memory that belongs to the thread.

**Q6.** (a) `syscall` stores the return address in `rcx` and the flags
in `r11`. Without the clobbers, the compiler may keep a live value in
`rcx` across the statement (for example a loop counter, or argument 4
of an inlined caller), and that value is silently destroyed. (b) Without
`"memory"`, the compiler assumes the asm touches only its operands. In
`char c = 'x'; write(1, &c, 1);` after inlining, the store to `c` can be
dropped as dead or moved after the trap. After `read(fd, buf, n)`, the
compiler can reuse a value of `buf[0]` that it loaded before the call.
(c) Without `volatile`, an asm whose outputs are unused may be deleted,
because the result of `exit_group` or `write` is often ignored. Two
identical calls may also be merged or hoisted out of a loop, since the
compiler treats the asm as a pure function of its inputs.

**Q7.** In a multi-threaded process, `sc_exit` must end *all* threads,
which is `exit_group` semantics. The last thread to go frees the process
resources: address space, loader, file descriptors. `pthread_exit` ends
only the caller. It stores the return value for `pthread_join`, wakes a
waiting joiner, and ends the process if it was the last thread.
Returning from `start_routine` must behave exactly like `pthread_exit`.
Killing another thread *immediately* is dangerous because it may be in
the middle of a system call. It might hold kernel locks (the memory
manager's, a file's) that would never be released, which deadlocks
everyone else, or it might be halfway through changing a kernel data
structure. Instead, set a "must die" flag and let the thread end itself
at a safe point: when it would return to user space, or when it is
scheduled while in user mode. A thread that sleeps in the kernel must
be woken so it can reach that point.

**Q8.**
1. `write()` (`userspace/libc/src/write.c`) calls
   `__syscall(sc_write, fd, buffer, count, 0, 0)`.
2. `__syscall` (`arch/x86/64/userspace/syscalls.c`) loads `rax = 4`,
   `rbx = 1`, `rcx = buffer`, `rdx = 2` and executes `int $0x80`.
3. **(a)** The CPU checks gate 0x80 (DPL 3), switches to ring 0 and to
   the thread's kernel stack, pushes the user `rip/cs/rflags/rsp/ss`,
   and jumps to `arch_syscallHandler`
   (`arch/x86/64/source/arch_interrupts.S`).
4. **(b)** `pushAll` + `arch_saveThreadRegisters` copy the user registers
   into `currentThread->user_registers_`.
5. `syscallHandler()` (`InterruptUtils.cpp`) sets
   `switch_to_userspace_ = 0`, enables interrupts and calls
   `Syscall::syscallException(rax, rbx, rcx, rdx, rsi, rdi)`.
6. `Syscall::syscallException` dispatches to `Syscall::write`, which
   checks the buffer and calls `kprintf`.
7. **(c)** Back in `syscallHandler`:
   `currentThread->user_registers_->rax = ret`. Interrupts go off,
   `switch_to_userspace_ = 1`, `currentThreadRegisters =
   user_registers_`, and `arch_contextSwitch()` restores them and
   returns to ring 3 (`iretq`) right after the `int $0x80`, where
   `__syscall` returns `rax`.

**Q9.**
| Argument | Dereferenced by the kernel? | Check |
|---|---|---|
| `thread` | **written**: the new thread id goes there | user range + writable, **before** creating the thread. Otherwise a thread exists that nobody can join (or you must undo it). Simpler design: the syscall returns the id and the libc wrapper stores it, so the kernel never touches the pointer. |
| `attr` | read, if you support attributes | `copy_from_user` once; for P1 it is usually NULL/ignored |
| `start_routine` | no, it becomes the new thread's instruction pointer | `< USER_BREAK` (a clean error instead of a thread that starts at a kernel address). It may still point to garbage, and then the new thread faults: the user's bug, like any bad function pointer. |
| `arg` | **no**, passed through in a register | **must not be checked**: NULL or any integer is a legal argument |

In practice the kernel starts a small user-space *wrapper* function
(passed in as another argument). The wrapper calls `start_routine(arg)`
and then `pthread_exit` with its result. Without it, returning from
`start_routine` would return to nowhere.
