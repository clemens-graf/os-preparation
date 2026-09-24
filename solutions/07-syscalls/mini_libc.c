/*
 * mini_libc.c - Module 07, part A: reference solution.
 */
#include "mini_libc.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>

__thread int ml_errno;

long ml_syscall6(long nr, long a1, long a2, long a3, long a4, long a5, long a6)
{
  /* No constraint letters exist for r10, r8, r9: pin local variables to
   * them. They must be set right before the asm statement (a function
   * call in between could clobber them). */
  register long r10 __asm__("r10") = a4;
  register long r8 __asm__("r8") = a5;
  register long r9 __asm__("r9") = a6;
  long ret;
  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(nr), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
                   : "rcx", "r11", "memory");
  return ret;
}

/* Raw result -> libc convention. The kernel never returns a valid value
 * in [-4095, -1]: not as a count, not as an fd, not as an address. */
static long ret_or_errno(long r)
{
  if ((unsigned long)r > -4096UL) {
    ml_errno = (int)-r;
    return -1;
  }
  return r;
}

ssize_t ml_write(int fd, const void *buf, size_t n)
{
  return ret_or_errno(ml_syscall6(SYS_write, fd, (long)buf, (long)n, 0, 0, 0));
}

ssize_t ml_read(int fd, void *buf, size_t n)
{
  return ret_or_errno(ml_syscall6(SYS_read, fd, (long)buf, (long)n, 0, 0, 0));
}

int ml_open(const char *path, int flags, int mode)
{
  /* openat(AT_FDCWD, ...) is what glibc uses; the mode is its 4th
   * argument and travels in r10. */
  return (int)ret_or_errno(ml_syscall6(SYS_openat, AT_FDCWD, (long)path, flags, mode, 0, 0));
}

int ml_close(int fd)
{
  return (int)ret_or_errno(ml_syscall6(SYS_close, fd, 0, 0, 0, 0, 0));
}

pid_t ml_getpid(void)
{
  /* getpid cannot fail. */
  return (pid_t)ml_syscall6(SYS_getpid, 0, 0, 0, 0, 0, 0);
}

void *ml_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  long r = ret_or_errno(ml_syscall6(SYS_mmap, (long)addr, (long)len, prot, flags, fd, offset));
  return r == -1 ? MAP_FAILED : (void *)r;
}

int ml_munmap(void *addr, size_t len)
{
  return (int)ret_or_errno(ml_syscall6(SYS_munmap, (long)addr, (long)len, 0, 0, 0, 0));
}

_Noreturn void ml_exit(int status)
{
  /* SYS_exit would end only this thread - which is what pthread_exit
   * uses. exit_group ends every thread of the process. */
  for (;;)
    ml_syscall6(SYS_exit_group, status, 0, 0, 0, 0, 0);
}
