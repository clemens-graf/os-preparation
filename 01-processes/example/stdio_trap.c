/*
 * stdio_trap.c - why output is sometimes printed twice after fork().
 *
 * printf() does not call write() immediately. It collects output in a
 * buffer inside the process (a plain array in user memory):
 *   - terminal:       flushed at every '\n'  (line buffered)
 *   - file or pipe:   flushed when the buffer is full or at exit (fully buffered)
 *
 * fork() copies the whole address space - including that buffer. If it
 * still holds unwritten text, parent and child each flush their own copy.
 *
 * Try:
 *     ./stdio_trap              (terminal: "hello" once)
 *     ./stdio_trap | cat        (pipe:     "hello" twice!)
 *     ./stdio_trap fixed | cat  (fflush before fork: once)
 */
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  printf("hello from PID %d\n", getpid());

  if (argc > 1 && strcmp(argv[1], "fixed") == 0)
    fflush(stdout);                 /* empty the buffer before duplicating it */

  pid_t pid = fork();
  if (pid == 0)
    return 0;                       /* returning from main flushes stdio */
  waitpid(pid, NULL, 0);
  return 0;
}
