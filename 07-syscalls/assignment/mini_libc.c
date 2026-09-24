/*
 * mini_libc.c - Module 07 assignment, part A. Replace the TODOs.
 * Test:  make test-libc
 *
 * Only headers from glibc, no functions: the constants in <sys/syscall.h>,
 * <errno.h>, <fcntl.h> and <sys/mman.h> are all you need. Look at
 * ../example/raw_syscall.c for a three-argument trap.
 */
#include "mini_libc.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>

__thread int ml_errno;

long ml_syscall6(long nr, long a1, long a2, long a3, long a4, long a5, long a6)
{
  /* TODO: one `syscall` instruction with all six arguments in the right
   * registers (see mini_libc.h), the result from rax. Do not forget the
   * clobbers: which registers does the instruction destroy, and may the
   * kernel read or write memory behind the compiler's back? */
  return -ENOSYS;
}

/* TODO (suggested): one helper that turns a raw result into the libc
 * convention - values in [-4095, -1] set ml_errno and become -1. */

ssize_t ml_write(int fd, const void *buf, size_t n)
{
  /* TODO */
  return -1;
}

ssize_t ml_read(int fd, void *buf, size_t n)
{
  /* TODO */
  return -1;
}

int ml_open(const char *path, int flags, int mode)
{
  /* TODO */
  return -1;
}

int ml_close(int fd)
{
  /* TODO */
  return -1;
}

pid_t ml_getpid(void)
{
  /* getpid cannot fail. */
  return (pid_t)ml_syscall6(SYS_getpid, 0, 0, 0, 0, 0, 0);
}

void *ml_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  /* TODO: six arguments. A mapping address is never in [-4095, -1],
   * so the same error test works here - but return MAP_FAILED. */
  return MAP_FAILED;
}

int ml_munmap(void *addr, size_t len)
{
  /* TODO */
  return -1;
}

_Noreturn void ml_exit(int status)
{
  /* TODO: SYS_exit ends only the calling THREAD. Which one ends the
   * whole process? */
  for (;;)
    __builtin_trap();
}
