# Module 07 — System calls and user pointers

**Time:** 3–4 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 7)

Your P1 task *is* a system call: `pthread_create` in SWEB's libc must trap
into the kernel, and the kernel must treat every argument as hostile. This
module does both sides — the user side for real on Linux, the kernel side
on a small simulated machine laid out exactly like SWEB's address space.

## Steps

1. **Read** chapter 7 (user/kernel mode, traps, the syscall convention on
   Linux and in SWEB, `-errno`, the path of one syscall through SWEB,
   user-pointer validation, overflow, TOCTOU / double fetch).
2. **Examples** — `cd example && make run`, then `make strace`:
   - `raw_syscall.c` — libc wrapper vs. `syscall()` vs. the bare instruction
   - `user_pointers.c` — the kernel's `EFAULT` vs. the program's `SIGSEGV`
   - `freestanding.c` — a program with no C library at all, its own `_start`
   - `hello.c` — how many syscalls `printf` really costs
3. **Assignment** — `assignment/`:
   - Part A `mini_libc.c`: `write`, `read`, `open`, `mmap`, `exit`, ...
     with inline-assembly `syscall` — no glibc functions allowed (checked)
   - Part B `kernel.c`: `access_ok`, `copy_from_user`, `copy_to_user`,
     `strncpy_from_user` on the simulated machine (`machine.h`)
   - Part C `kernel.c`: the syscall handlers for `read`, `write`, `open`,
     `close` behind a SWEB-style register dispatcher; bonus: `writev`
   ```bash
   cd assignment && make test     # or test-libc / test-uaccess / test-syscalls
   ```
   Parts B and C run under AddressSanitizer: a copy that runs over a page
   boundary shows up as a heap-buffer-overflow.
4. **Paper questions** — `assignment/questions.md` (Q2, Q7 and Q9 are
   about your own SWEB code).

## Done when

- `make test` prints `==== all parts pass` (bonus: no SKIPs).
- You can explain why `buffer + size > USER_BREAK` is not a check, and
  which `pthread_create` arguments the kernel must — and must not — check.

Reference solution and answers: `../solutions/07-syscalls/`.
