/*
 * freestanding.c - a complete program without any C library.
 *
 * Built with -nostdlib -static: no printf, no startup code (crt1.o), no
 * libc at all. The kernel starts the program at the ELF entry point
 * `_start`, with the stack pointer pointing at argc, followed by the argv
 * pointers. _start cannot return - there is nothing to return to. The only
 * way out is the exit system call.
 *
 * SWEB user programs work the same way: userspace/libc/src/nonstd.c
 * defines _start(), which calls main() and passes its result to exit(),
 * i.e. to the sc_exit system call.
 *
 * Try:  make freestanding && ./freestanding a b; echo $?
 *       strace ./freestanding        (compare with `make strace`)
 */
#include <sys/syscall.h>     /* only #defines: SYS_write, SYS_exit_group */

static long sys3(long nr, long a1, long a2, long a3)
{
  long ret;
  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(nr), "D"(a1), "S"(a2), "d"(a3)
                   : "rcx", "r11", "memory");
  return ret;
}

static unsigned long my_strlen(const char *s)
{
  unsigned long n = 0;
  while (s[n])
    n++;
  return n;
}

static void print(const char *s)
{
  sys3(SYS_write, 1, (long)s, (long)my_strlen(s));
}

int main(int argc, char **argv)
{
  print("hello from a program without libc\n");
  for (int i = 0; i < argc; i++) {
    print("  argv: ");
    print(argv[i]);
    print("\n");
  }
  return argc;                 /* becomes the exit status */
}

/* The entry point, in assembly: there is no C stack frame to rely on yet.
 * The ABI wants rsp 16-byte aligned at every call. */
__asm__(".global _start\n"
        "_start:\n"
        "  xor  %rbp, %rbp\n"           /* marks the outermost frame */
        "  mov  (%rsp), %rdi\n"         /* argc */
        "  lea  8(%rsp), %rsi\n"        /* argv */
        "  and  $-16, %rsp\n"
        "  call main\n"
        "  mov  %eax, %edi\n"           /* exit status */
        "  mov  $" "231" ", %eax\n"     /* SYS_exit_group */
        "  syscall\n"
        "  hlt\n");                     /* never reached */
