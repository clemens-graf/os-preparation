/*
 * kernel.h - Module 07 assignment, parts B and C: the kernel side of a
 * system call, on the simulated machine of machine.h.
 *
 * Every argument of a system call is chosen by the user program, i.e. by
 * an attacker. Numbers can be anything; a "pointer" is just a number the
 * user claims is the address of a buffer. The kernel must make sure that
 *   - the whole range is USER memory (below USER_BREAK): otherwise
 *     write(1, kernel_address, 100) prints kernel memory;
 *   - the arithmetic that checks this cannot overflow;
 *   - every page of the range is mapped (and writable, for output buffers);
 *   - it never reads the same user value twice and trusts it both times.
 *
 * Linux's names for these helpers are used below; SWEB checks
 * `buffer + size > USER_BREAK` by hand in each Syscall:: method.
 *
 * Errors are returned as -errno, like the Linux kernel does.
 */
#pragma once
#include <stddef.h>
#include "machine.h"

/* ------------------ Part B: touching user memory safely ------------------ */

/*
 * 1 if [uaddr, uaddr + len) lies completely below USER_BREAK, else 0.
 * Pure arithmetic - no page look-ups - and correct for EVERY pair of
 * inputs, including those where uaddr + len does not fit into a size_t.
 * A range of length 0 is OK iff uaddr <= USER_BREAK.
 */
int access_ok(size_t uaddr, size_t len);

/*
 * Copy len bytes from user address usrc into the kernel buffer dst.
 * 0 on success; -EFAULT if the range is not user memory or any page of it
 * is not mapped. Kernel memory must not even be looked up (mmu_translate)
 * on behalf of a bad range. On -EFAULT the contents of dst are undefined.
 */
long copy_from_user(void *dst, size_t usrc, size_t len);

/*
 * Copy len bytes from the kernel buffer src to user address udst.
 * 0 on success; -EFAULT if the range is not user memory, or any page of it
 * is unmapped or read-only. All or nothing: after -EFAULT, not a single
 * byte of user memory has changed. (Linux may copy a prefix and return the
 * number of bytes it could NOT copy; this simpler rule makes testing easier.)
 */
long copy_to_user(size_t udst, const void *src, size_t len);

/*
 * Copy the NUL-terminated string at user address usrc into the kernel
 * buffer dst of size max (the NUL included).
 * Returns the string length (without the NUL);
 *   -EFAULT       if a byte up to and including the NUL is not readable
 *                 user memory,
 *   -ENAMETOOLONG if the first max bytes are readable but contain no NUL.
 * Unlike for a buffer, the length is not known in advance: you cannot
 * validate the range first.
 */
long strncpy_from_user(char *dst, size_t usrc, size_t max);

/* ------------------ Part C: the system call dispatcher ------------------ */

/* SWEB's numbers (common/include/kernel/syscall-definitions.h) */
#define SC_READ   3     /* read(fd, buf, n)    fd 0 = keyboard, fds 3..9 = files */
#define SC_WRITE  4     /* write(fd, buf, n)   only fd 1 = console              */
#define SC_OPEN   5     /* open(path)          -> fd 3..9                       */
#define SC_CLOSE  6     /* close(fd)                                            */
#define SC_WRITEV 20    /* bonus: writev(fd, iov, iovcnt), only fd 1            */

#define KPATH_MAX 64    /* paths longer than 63 characters: -ENAMETOOLONG */

/* The user's `struct iovec` as it lies in user memory (writev). */
struct uiovec {
  size_t base;          /* user address */
  size_t len;
};
#define UIO_MAXIOV 16   /* more entries: -EINVAL */

/*
 * The user registers saved by the trap, in SWEB's x86-64 layout
 * (arch/x86/64/source/InterruptUtils.cpp, syscallHandler):
 * rax = number, rbx, rcx, rdx, rsi, rdi = arguments 1..5.
 */
struct regs {
  size_t rax, rbx, rcx, rdx, rsi, rdi;
};

/*
 * Handle the system call described by r and put the result into r->rax:
 * a value >= 0 on success, otherwise -errno (stored as a size_t).
 *   unknown number             -> -ENOSYS
 *   fd not valid for the call  -> -EBADF
 *   bad user pointer           -> -EFAULT
 *
 * read and write may transfer fewer bytes than asked ("short" read/write)
 * and return that count. In particular, if a buffer passes access_ok but
 * becomes unmapped part-way, write returns the number of bytes it wrote
 * (the console must have received exactly those) or -EFAULT if it wrote
 * nothing. Never allocate a kernel buffer of a user-chosen size.
 *
 * Bonus writev: fd 1 only; reject iovcnt > UIO_MAXIOV (-EINVAL); fetch
 * the whole iovec array with ONE copy_from_user into a kernel array; check
 * every entry with access_ok (-EFAULT) BEFORE writing anything; then write
 * the entries in order (same short-write rule) and return the total.
 */
void syscall_entry(struct regs *r);
