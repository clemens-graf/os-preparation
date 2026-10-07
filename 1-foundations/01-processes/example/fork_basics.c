/*
 * fork_basics.c - what fork() really does.
 *
 * fork() creates a new process that is an (almost) exact copy of the
 * caller: same code, same variable values, same open files, and it
 * continues at the same place - right after the fork() call.
 * The only difference visible to the program is the return value:
 *
 *     > 0   in the parent: the child's PID
 *     == 0  in the child
 *     < 0   fork failed (no child was created)
 *
 * After the fork the two processes have *separate* address spaces:
 * a write in one is invisible to the other. (Internally the kernel shares
 * the pages copy-on-write until someone writes - see module 08.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int global = 10;

int main(void)
{
  int local = 20;
  int *heap = malloc(sizeof *heap);
  *heap = 30;

  printf("before fork: I am PID %d\n", getpid());
  fflush(stdout);   /* see stdio_trap.c for why this line matters */

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  }

  if (pid == 0) {
    /* ---------------- child ---------------- */
    global++;
    local++;
    (*heap)++;
    printf("child : PID %d, parent %d, global=%d local=%d heap=%d\n",
           getpid(), getppid(), global, local, *heap);
    printf("child : &global=%p (same virtual address as in the parent!)\n", (void *)&global);
    free(heap);
    exit(7);        /* exit status 7 goes to the parent via wait */
  }

  /* ---------------- parent ---------------- */
  int status;
  waitpid(pid, &status, 0);   /* block until that child terminates */

  printf("parent: PID %d, child was %d, global=%d local=%d heap=%d (unchanged)\n",
         getpid(), pid, global, local, *heap);
  printf("parent: &global=%p\n", (void *)&global);
  if (WIFEXITED(status))
    printf("parent: child exited with status %d\n", WEXITSTATUS(status));

  /* Same virtual address, different values: each process has its own
   * page table mapping that address to a different physical frame. */
  free(heap);
  return 0;
}
