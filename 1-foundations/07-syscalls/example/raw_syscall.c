/*
 * raw_syscall.c - three ways to ask the kernel to write "hello".
 *
 * User programs cannot touch hardware or other processes' memory; the CPU
 * refuses in user mode (ring 3). A system call is the controlled door
 * into the kernel (ring 0): put a number and arguments into registers,
 * execute a special trap instruction, the CPU switches to kernel mode and
 * jumps to ONE fixed entry point the kernel registered at boot. The kernel
 * looks at the number, does the work, puts a result in a register and
 * returns to the instruction after the trap.
 *
 * Linux x86-64 convention (instruction: `syscall`):
 *     rax = number   rdi, rsi, rdx, r10, r8, r9 = arguments 1..6
 *     result in rax; errors as -errno (e.g. -9 = -EBADF)
 *     the instruction itself overwrites rcx and r11
 *
 * SWEB x86-64 convention (instruction: `int $0x80`, see
 * arch/x86/64/userspace/syscalls.c in your repo):
 *     rax = number   rbx, rcx, rdx, rsi, rdi = arguments 1..5
 *     result in rax
 * Same idea, different registers and trap instruction.
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

/* The raw trap with three arguments. "memory" clobber: the kernel may read
 * or write memory we point to (the buffer), so the compiler must not keep
 * such values cached in registers across this statement. */
static long raw_syscall3(long nr, long a1, long a2, long a3)
{
  long ret;
  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(nr), "D"(a1), "S"(a2), "d"(a3)
                   : "rcx", "r11", "memory");
  return ret;
}

int main(void)
{
  const char msg[] = "hello\n";

  /* 1. libc wrapper: sets errno and returns -1 on error */
  write(STDOUT_FILENO, "1) libc write:     ", 19);
  write(STDOUT_FILENO, msg, sizeof msg - 1);

  /* 2. generic libc syscall(): number + arguments, still errno-style */
  syscall(SYS_write, STDOUT_FILENO, "2) syscall():      ", 19);
  syscall(SYS_write, STDOUT_FILENO, msg, sizeof msg - 1);

  /* 3. the bare instruction: returns -errno directly */
  raw_syscall3(SYS_write, STDOUT_FILENO, (long)"3) raw syscall:    ", 19);
  raw_syscall3(SYS_write, STDOUT_FILENO, (long)msg, sizeof msg - 1);

  printf("\nSYS_write = %d, SYS_getpid = %d, SYS_exit_group = %d on this machine\n",
         SYS_write, SYS_getpid, SYS_exit_group);
  printf("raw getpid: %ld, libc getpid: %d\n", raw_syscall3(SYS_getpid, 0, 0, 0), getpid());

  /* Errors: the raw call returns -errno; libc turns that into -1 + errno. */
  long r = raw_syscall3(SYS_write, 42, (long)msg, 1);     /* fd 42 is not open */
  printf("raw write to fd 42  -> %ld  (-EBADF = %d)\n", r, -EBADF);
  errno = 0;
  ssize_t w = write(42, msg, 1);
  printf("libc write to fd 42 -> %zd, errno = %d (%s)\n", w, errno, strerror(errno));
  return 0;
}
