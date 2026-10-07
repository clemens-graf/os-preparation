/*
 * pipe_demo.c - connecting two programs like the shell does for `ls / | wc -l`.
 *
 * File descriptors (fds) are small integers indexing a per-process table
 * of open files. 0 = stdin, 1 = stdout, 2 = stderr by convention.
 * fork() copies the table; exec() keeps it. That is the whole trick:
 *
 *   pipe(fds)        -> fds[0] = read end, fds[1] = write end
 *   child 1: dup2(fds[1], 1)  stdout now goes into the pipe, then exec ls
 *   child 2: dup2(fds[0], 0)  stdin now comes from the pipe, then exec wc
 *
 * The one rule people forget: close every pipe end you do not use.
 * `wc` only sees end-of-file when *all* write ends are closed - if the
 * parent (or wc itself!) still holds a write end, wc waits forever.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
  int fds[2];
  if (pipe(fds) < 0) {
    perror("pipe");
    return 1;
  }

  fflush(stdout);
  pid_t writer = fork();
  if (writer == 0) {
    dup2(fds[1], STDOUT_FILENO);    /* fd 1 := write end of the pipe */
    close(fds[0]);                  /* not needed in this process    */
    close(fds[1]);                  /* fd 1 is a copy, drop the original */
    execlp("ls", "ls", "/", (char *)NULL);
    _exit(127);
  }

  pid_t reader = fork();
  if (reader == 0) {
    dup2(fds[0], STDIN_FILENO);     /* fd 0 := read end of the pipe  */
    close(fds[0]);
    close(fds[1]);                  /* <- forget this and wc hangs forever */
    execlp("wc", "wc", "-l", (char *)NULL);
    _exit(127);
  }

  /* The parent uses neither end. Close both, otherwise wc never sees EOF. */
  close(fds[0]);
  close(fds[1]);

  waitpid(writer, NULL, 0);
  waitpid(reader, NULL, 0);
  printf("(the number above is the number of entries in /)\n");
  return 0;
}
