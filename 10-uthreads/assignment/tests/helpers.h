/* Shared by the module 10 tests. */
#pragma once
#include "check.h"
#include "uthread.h"

#include <signal.h>
#include <sys/wait.h>

/* ---- watchdog: a hanging test ends the program with a message ---- */
static void on_hang(int sig)
{
  static const char msg[] =
      "    FAIL: the test did not finish within 10 s - a thread that is never woken up,\n"
      "          or one that spins without ever yielding?\n";
  (void)sig;
  if (write(STDERR_FILENO, msg, sizeof msg - 1) < 0) {}
  _exit(3);
}

#define RUN(test)                                                               \
  do {                                                                          \
    signal(SIGALRM, on_hang);                                                   \
    alarm(10);                                                                  \
    RUN_TEST(test);                                                             \
    alarm(0);                                                                   \
  } while (0)

/* ---- a log of what happened in which order ---- */
static char trace[1024];
static int trace_len;

static void __attribute__((unused)) note(char c)
{
  if (trace_len < (int)sizeof trace - 1) {
    trace[trace_len++] = c;
    trace[trace_len] = '\0';
  }
}

static void __attribute__((unused)) trace_reset(void)
{
  trace_len = 0;
  trace[0] = '\0';
}

#define CHECK_TRACE(want) CHECK_STR(trace, want)

/* ---- run fn in a child process (for tests that end the process) ----
 * Returns the wait status; the child's stdout/stderr go to the pipe whose
 * contents land in out (NUL-terminated). */
static int __attribute__((unused)) in_child(void (*fn)(void), char *out, size_t outsize)
{
  int p[2];
  if (pipe(p) != 0) {
    perror("pipe");
    exit(2);
  }
  fflush(NULL);
  pid_t pid = fork();
  if (pid == 0) {
    alarm(10);                     /* SIGALRM's default action kills a hanging child */
    signal(SIGALRM, SIG_DFL);
    close(p[0]);
    dup2(p[1], STDOUT_FILENO);
    dup2(p[1], STDERR_FILENO);
    fn();
    fflush(NULL);
    _exit(0);                      /* fn returned (it was supposed to end the process) */
  }
  close(p[1]);
  size_t n = 0;
  ssize_t r;
  while (n + 1 < outsize && (r = read(p[0], out + n, outsize - 1 - n)) > 0)
    n += (size_t)r;
  out[n] = '\0';
  close(p[0]);
  int status;
  waitpid(pid, &status, 0);
  return status;
}

static __attribute__((unused)) const char *describe(int status)
{
  static char buf[64];
  if (WIFEXITED(status))
    snprintf(buf, sizeof buf, "exit status %d", WEXITSTATUS(status));
  else if (WIFSIGNALED(status))
    snprintf(buf, sizeof buf, "killed by signal %d (%s)", WTERMSIG(status),
             strsignal(WTERMSIG(status)));
  else
    snprintf(buf, sizeof buf, "status %#x", status);
  return buf;
}
