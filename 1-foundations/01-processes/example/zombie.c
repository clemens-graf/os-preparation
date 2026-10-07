/*
 * zombie.c - zombies and orphans, made visible.
 *
 * When a process terminates, the kernel frees almost everything (memory,
 * files) but keeps a tiny record: PID + exit status. That record stays
 * until the parent collects it with wait()/waitpid(). A terminated child
 * whose status was not collected yet is a *zombie* (state 'Z' in ps).
 *
 * If the parent dies first, the child becomes an *orphan* and is adopted
 * by init (PID 1) or a "subreaper", which will wait() for it eventually.
 *
 * pthread_join is the thread-world version of the same idea: a finished
 * thread keeps its return value around until someone joins it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static char state_of(pid_t pid)
{
  char path[64], buf[256];
  snprintf(path, sizeof path, "/proc/%d/stat", pid);
  FILE *f = fopen(path, "r");
  if (!f)
    return '?';                     /* no such process any more */
  if (!fgets(buf, sizeof buf, f)) {
    fclose(f);
    return '?';
  }
  fclose(f);
  /* format: "pid (comm) S ..." - the state letter follows the ')' */
  char *close_paren = strrchr(buf, ')');
  return close_paren ? close_paren[2] : '?';
}

int main(void)
{
  fflush(stdout);
  pid_t pid = fork();
  if (pid == 0)
    _exit(3);                       /* child terminates immediately */

  sleep(1);                         /* child is dead, but not yet waited for */
  printf("child %d state before waitpid: %c  (Z = zombie)\n", pid, state_of(pid));

  int status;
  waitpid(pid, &status, 0);         /* collect the exit status -> record freed */
  printf("child %d state after  waitpid: %c  (? = gone)\n", pid, state_of(pid));
  printf("exit status was %d\n", WEXITSTATUS(status));
  return 0;
}
