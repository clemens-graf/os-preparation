/*
 * user_pointers.c - the kernel never trusts a pointer from user space.
 *
 * write(fd, buf, len) makes the KERNEL read len bytes at address buf. If
 * the kernel dereferenced a garbage pointer blindly, a user program could
 *   - crash the kernel (unmapped address -> page fault inside the kernel),
 *   - read kernel memory (buf pointing into the kernel's half of the
 *     address space) and write it to a file: a complete security hole.
 * So every syscall checks user pointers first. Linux returns EFAULT.
 * SWEB checks `buffer + size > USER_BREAK` in Syscall::write & co. -
 * the kernel lives above USER_BREAK. (Module 07's assignment asks you to
 * find the flaw in exactly that check.)
 *
 * Compare: the same bad pointer dereferenced by the PROGRAM itself kills
 * only the program (SIGSEGV), never the kernel.
 */
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void try_write(const char *what, const void *buf, size_t len)
{
  fflush(stdout);              /* keep printf's buffer and the raw write in order */
  errno = 0;
  ssize_t r = write(STDOUT_FILENO, buf, len);
  printf("  write(%-28s) -> %zd, errno = %s\n", what, r, errno ? strerror(errno) : "0");
}

int main(void)
{
  printf("The kernel validates pointers passed to syscalls:\n");
  try_write("NULL, 5", NULL, 5);
  try_write("(void *)1, 5", (void *)1, 5);
  /* An address in the kernel half of the address space (top bit set): */
  try_write("0xffffffff81000000, 5", (void *)(uintptr_t)0xffffffff81000000ull, 5);
  try_write("\"ok\\n\", 3", "ok\n", 3);

  printf("\nThe same bad pointer used by the program itself:\n");
  fflush(stdout);
  pid_t pid = fork();
  if (pid == 0) {
    volatile char *p = (char *)1;
    char c = *p;                     /* the CPU raises a page fault ... */
    (void)c;
    _exit(0);
  }
  int status;
  waitpid(pid, &status, 0);
  if (WIFSIGNALED(status))
    printf("  child killed by signal %d (%s) - the kernel survived, of course\n",
           WTERMSIG(status), strsignal(WTERMSIG(status)));
  return 0;
}
