/*
 * minish.c - Module 01 assignment: a minimal shell.
 *
 * Build & test:   make test        Try it by hand:   make && ./minish
 *
 * ---------------------------------------------------------------------
 * Specification
 * ---------------------------------------------------------------------
 * minish reads commands from stdin, one per line, until end of input.
 * Words are separated by blanks; there are no quotes, no variables, no
 * globbing. The line-reading loop in main() is already written for you.
 *
 * 1. External commands  (implement run_external)
 *    Run the command in a child process with fork + execvp and wait for it.
 *    The *status* of a command is
 *      - its exit code if it exited normally,
 *      - 128 + signal number if it was killed by a signal.
 *    If execvp fails, the child prints
 *          minish: <command>: command not found
 *    to stderr and terminates with status 127.
 *    No zombies: every child must be waited for.
 *
 * 2. Builtin  cd [dir]  (implement builtin_cd)
 *    Change the shell's working directory to dir, or to $HOME without an
 *    argument. On failure print
 *          minish: cd: <dir>: <strerror(errno)>
 *    to stderr; the status is then 1, otherwise 0.
 *    Think first: why can `cd` NOT be an external program?
 *
 * 3. Builtin  exit [n]  (already done in main)
 *
 * 4. After every command with a non-zero status, main() prints
 *          [status N]
 *    on stdout (already done). Make sure this line appears in the right
 *    order relative to the output of the commands (hint: chapter 1,
 *    "stdio buffers and fork").
 *
 * 5. BONUS: a single pipe  "cmd1 args | cmd2 args"  (implement run_pipeline)
 *    '|' is its own word. Both commands run concurrently; the status is
 *    the status of the right-hand command. Close every unused pipe end!
 * ---------------------------------------------------------------------
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* ===================================================================== */
/* Your part                                                             */
/* ===================================================================== */

/* REFERENCE SOLUTION */

/* Turn a raw waitpid() status into the shell-style status. */
static int decode_status(int raw)
{
  if (WIFEXITED(raw))
    return WEXITSTATUS(raw);
  if (WIFSIGNALED(raw))
    return 128 + WTERMSIG(raw);
  return 1;
}

static int wait_for(pid_t pid)
{
  int raw;
  while (waitpid(pid, &raw, 0) < 0) {
    if (errno != EINTR) {
      perror("minish: waitpid");
      return 1;
    }
  }
  return decode_status(raw);
}

/* Only ever called in a child: never returns. */
static void exec_or_die(char **argv)
{
  execvp(argv[0], argv);
  fprintf(stderr, "minish: %s: command not found\n", argv[0]);
  /* _exit, not exit. exit() runs the stdio cleanup on the child's *copy*
   * of the parent's FILE buffers:
   *   - stdout: unwritten output would be written a second time;
   *   - stdin:  glibc rewinds the file offset to the position the program
   *             has logically read up to. Parent and child share that
   *             offset (same open file after fork), so the parent later
   *             re-reads lines it had already buffered -> commands run twice
   *             (test 05 catches exactly this).
   * It would also run atexit handlers that belong to the parent. */
  _exit(127);
}

/* Run an external command. argv is NULL-terminated, argv[0] is the
 * command. Returns the status as defined in the specification. */
static int run_external(char **argv)
{
  /* Anything still sitting in our stdout buffer would otherwise be written
   * *after* the child's output (wrong order), or twice if the child flushes
   * its copy of the buffer. */
  fflush(stdout);

  pid_t pid = fork();
  if (pid < 0) {
    perror("minish: fork");
    return 1;
  }
  if (pid == 0)
    exec_or_die(argv);
  return wait_for(pid);
}

/* The builtin cd. argv[0] is "cd". Returns the status.
 * cd must be a builtin: the working directory is per-process state, and a
 * child can never change its parent's state - it only has a copy. */
static int builtin_cd(char **argv)
{
  const char *dir = argv[1] ? argv[1] : getenv("HOME");
  if (!dir)
    dir = "/";
  if (chdir(dir) < 0) {
    fprintf(stderr, "minish: cd: %s: %s\n", dir, strerror(errno));
    return 1;
  }
  return 0;
}

/* BONUS: run left | right. Both are NULL-terminated argument vectors.
 * Returns the status of the right-hand command. */
static int run_pipeline(char **left, char **right)
{
  int fds[2];
  if (pipe(fds) < 0) {
    perror("minish: pipe");
    return 1;
  }
  fflush(stdout);

  pid_t lpid = fork();
  if (lpid == 0) {
    dup2(fds[1], STDOUT_FILENO);
    close(fds[0]);
    close(fds[1]);
    exec_or_die(left);
  }

  pid_t rpid = fork();
  if (rpid == 0) {
    dup2(fds[0], STDIN_FILENO);
    close(fds[0]);
    close(fds[1]);       /* otherwise the reader never sees end-of-file */
    exec_or_die(right);
  }

  /* The shell uses neither end. Closing them here is what lets the right
   * command see EOF once the left one exits. */
  close(fds[0]);
  close(fds[1]);

  if (lpid > 0)
    wait_for(lpid);
  return rpid > 0 ? wait_for(rpid) : 1;
}

/* ===================================================================== */
/* Provided: reading, splitting and dispatching lines. No need to edit.  */
/* ===================================================================== */

static char **split_line(char *line, size_t *count)
{
  size_t cap = 8, n = 0;
  char **words = malloc(cap * sizeof *words);
  if (!words)
    return NULL;
  for (char *tok = strtok(line, " \t\n"); tok; tok = strtok(NULL, " \t\n")) {
    if (n + 1 >= cap) {
      char **bigger = realloc(words, (cap *= 2) * sizeof *words);
      if (!bigger) {
        free(words);
        return NULL;
      }
      words = bigger;
    }
    words[n++] = tok;                 /* points into `line`, no copies */
  }
  words[n] = NULL;
  *count = n;
  return words;
}

static int execute(char **words, size_t n)
{
  if (strcmp(words[0], "cd") == 0)
    return builtin_cd(words);

  for (size_t i = 0; i < n; i++) {
    if (strcmp(words[i], "|") == 0) {
      if (i == 0 || i + 1 == n) {
        fprintf(stderr, "minish: syntax error near '|'\n");
        return 2;
      }
      words[i] = NULL;                /* split into two argv arrays */
      return run_pipeline(words, &words[i + 1]);
    }
  }
  return run_external(words);
}

int main(void)
{
  int interactive = isatty(STDIN_FILENO);
  char *line = NULL;
  size_t cap = 0;
  int status = 0;

  for (;;) {
    if (interactive) {
      printf("minish$ ");
      fflush(stdout);
    }
    if (getline(&line, &cap, stdin) < 0)
      break;                          /* end of input */

    size_t n = 0;
    char **words = split_line(line, &n);
    if (!words)
      break;
    if (n == 0) {
      free(words);
      continue;
    }

    if (strcmp(words[0], "exit") == 0) {
      int code = n > 1 ? atoi(words[1]) : status;
      free(words);
      free(line);
      return code;
    }

    status = execute(words, n);
    if (status != 0)
      printf("[status %d]\n", status);
    free(words);
  }

  free(line);
  return status;
}
