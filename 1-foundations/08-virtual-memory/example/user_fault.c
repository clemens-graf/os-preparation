/*
 * user_fault.c - a page-fault handler in user space.
 *
 * The kernel's page-fault handler does: find out which address faulted,
 * decide whether the access is legal, map a page, return - and the CPU
 * RE-EXECUTES the faulting instruction, which now succeeds. We can play
 * the same game one level up: reserve memory with PROT_NONE (every access
 * faults), catch SIGSEGV, make the page accessible with mprotect(), fill
 * it, return from the handler. The program never notices.
 *
 * (Real programs rarely do this; garbage collectors and the userfaultfd
 * mechanism use the idea. mprotect is not formally async-signal-safe, but
 * it is a plain system call on Linux.)
 */
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGES 8

static char *region;
static size_t page;
static volatile int faults;

static void on_segv(int sig, siginfo_t *si, void *ctx)
{
  char *addr = si->si_addr;
  if (addr < region || addr >= region + PAGES * page) {
    /* Not ours: a genuine bug. Restore the default action and return:
     * the instruction faults again and the process dies as usual. */
    signal(SIGSEGV, SIG_DFL);
    return;
  }
  char *pg = (char *)((uintptr_t)addr & ~(page - 1));
  mprotect(pg, page, PROT_READ | PROT_WRITE);           /* "map" the page */
  memset(pg, 'a' + (int)((pg - region) / page), page);  /* "load" its contents */
  faults++;
}

int main(void)
{
  page = (size_t)sysconf(_SC_PAGESIZE);
  region = mmap(NULL, PAGES * page, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (region == MAP_FAILED) {
    perror("mmap");
    return 1;
  }
  struct sigaction sa = {.sa_sigaction = on_segv, .sa_flags = SA_SIGINFO};
  sigemptyset(&sa.sa_mask);
  sigaction(SIGSEGV, &sa, NULL);

  printf("reading one byte from pages 3, 0, 3, 7, 0:\n  ");
  int order[] = {3, 0, 3, 7, 0};
  for (int i = 0; i < 5; i++)
    printf("%c ", region[order[i] * page + 100]);
  printf("\n  -> %d faults for 5 accesses: each page faults once, then it is mapped\n",
         faults);

  printf("now a real bug (NULL dereference) - the handler lets it through:\n");
  fflush(stdout);
  volatile char *null = NULL;
  return *null;
}
