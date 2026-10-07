/*
 * mini_libc.h - Module 07 assignment, part A: a C library without a C library.
 *
 * Every libc function that touches the outside world ends in a system call.
 * Here you write that last step yourself, for real, on Linux x86-64: no
 * write(), no syscall(), no errno from glibc - just the `syscall`
 * instruction. `make test-libc` checks that mini_libc.o has NO undefined
 * symbols, i.e. that it calls nothing from glibc.
 *
 * Linux x86-64 system call convention:
 *     rax = number, arguments in rdi, rsi, rdx, r10, r8, r9 (in this order)
 *     result in rax; the `syscall` instruction overwrites rcx and r11
 *     an error comes back as a value in [-4095, -1], i.e. -errno
 *
 * GCC has constraint letters for rax/rdi/rsi/rdx ("a", "D", "S", "d") but
 * NOT for r10, r8 and r9. Use local register variables for those:
 *     register long r10 __asm__("r10") = a4;
 * and pass them with the "r" constraint. (Careful: the C function calling
 * convention passes argument 4 in rcx - the syscall one uses r10, because
 * `syscall` itself destroys rcx.)
 *
 * Syscall numbers: <sys/syscall.h> (a header with #defines only, no code).
 */
#pragma once
#include <stddef.h>
#include <sys/types.h>

/*
 * errno of this library. Thread-local: every thread gets its own copy
 * (see module 02). With a single global, thread 1's failing call could
 * overwrite the errno that thread 2 is about to inspect.
 */
extern __thread int ml_errno;

/* The raw trap: returns what the kernel put in rax (>= 0 or -errno).
 * Unused arguments may be anything. Does not touch ml_errno. */
long ml_syscall6(long nr, long a1, long a2, long a3, long a4, long a5, long a6);

/*
 * The wrappers behave like their libc counterparts: on error they set
 * ml_errno and return -1 (ml_mmap returns MAP_FAILED, i.e. (void *)-1).
 * On success they leave ml_errno alone.
 */
ssize_t ml_write(int fd, const void *buf, size_t n);
ssize_t ml_read(int fd, void *buf, size_t n);
int     ml_open(const char *path, int flags, int mode);   /* SYS_open or SYS_openat */
int     ml_close(int fd);
pid_t   ml_getpid(void);
void   *ml_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
int     ml_munmap(void *addr, size_t len);

/* Ends the whole PROCESS, all of its threads, with the given status.
 * (Linux has two exit calls; pick the right one. Must never return.) */
_Noreturn void ml_exit(int status);
