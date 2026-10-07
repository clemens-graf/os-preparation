/*
 * exec_wait.c - fork + exec + wait: how every shell starts a program.
 *
 * exec*() does NOT create a process. It replaces the program running in
 * the *current* process with a new one: new code, new data, new stack,
 * fresh heap. The PID stays the same, open file descriptors stay open.
 * If exec succeeds it never returns (the code that called it is gone).
 *
 *   parent                          child
 *   ------                          -----
 *   pid = fork()  ----------------> (copy of parent)
 *   waitpid(pid)  ... blocks ...    execvp("ls", argv)  -> now it *is* ls
 *                                   ls runs, calls exit(0)
 *   <- waitpid returns, status 0 -- (process terminates)
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Runs argv[0] with the given arguments and returns a shell-style status:
 * the exit code, or 128 + signal number if the child was killed. */
static int run(char *const argv[])
{
  fflush(stdout);
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return -1;
  }
  if (pid == 0) {
    execvp(argv[0], argv);          /* searches $PATH like a shell does */
    /* Only reached if exec failed. Report and leave *immediately* with
     * _exit: exit() would flush stdio buffers copied from the parent and
     * run the parent's atexit handlers inside the child. */
    fprintf(stderr, "exec %s failed: %s\n", argv[0], strerror(errno));
    _exit(127);                     /* 127 = "command not found" by convention */
  }

  int status;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR) {           /* interrupted by a signal: just retry */
      perror("waitpid");
      return -1;
    }
  }
  if (WIFEXITED(status))
    return WEXITSTATUS(status);     /* normal exit: exit(n) / return n from main */
  if (WIFSIGNALED(status))
    return 128 + WTERMSIG(status);  /* killed, e.g. SIGSEGV -> 139, SIGKILL -> 137 */
  return -1;
}

int main(void)
{
  char *ls[] = {"ls", "-1", "/", NULL};          /* argv must end with NULL */
  char *fail[] = {"false", NULL};
  char *missing[] = {"no-such-program-xyz", NULL};
  char *crash[] = {"sh", "-c", "kill -SEGV $$", NULL};

  printf("status of ls:      %d\n", run(ls));
  printf("status of false:   %d\n", run(fail));
  printf("status of missing: %d\n", run(missing));
  printf("status of crash:   %d (128 + SIGSEGV=11)\n", run(crash));
  return 0;
}
