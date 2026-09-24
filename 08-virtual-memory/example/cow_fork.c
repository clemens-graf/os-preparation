/*
 * cow_fork.c - fork() copies nothing until somebody writes.
 *
 * After fork, parent and child share every frame, all mapped read-only
 * with a copy-on-write mark (the assignment's as_fork). Reading costs
 * nothing. The first WRITE to a page faults, and only then is that one
 * page copied. The minor-fault counts show it page by page.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SIZE (64UL << 20)

static long minor_faults(void)
{
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  return ru.ru_minflt;
}

static double ms_since(struct timespec *t0)
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (t.tv_sec - t0->tv_sec) * 1e3 + (t.tv_nsec - t0->tv_nsec) / 1e6;
}

int main(void)
{
  size_t page = (size_t)sysconf(_SC_PAGESIZE);
  char *buf = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (buf == MAP_FAILED) {
    perror("mmap");
    return 1;
  }
  memset(buf, 'P', SIZE);                     /* all 16384 pages present */
  printf("parent: %lu MiB = %lu pages, all written\n", SIZE >> 20, SIZE / page);

  fflush(stdout);                             /* or the child inherits the buffer (module 01) */
  struct timespec t0;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  pid_t pid = fork();
  if (pid == 0) {
    printf("child:  fork took %.2f ms (no data copied)\n", ms_since(&t0));
    long f0 = minor_faults();
    long sum = 0;
    for (size_t off = 0; off < SIZE; off += page)
      sum += buf[off];
    printf("child:  read every page:  %6ld minor faults\n", minor_faults() - f0);

    f0 = minor_faults();
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (size_t off = 0; off < SIZE; off += page)
      buf[off] = 'c';
    printf("child:  wrote every page: %6ld minor faults, %.1f ms (one copy per page)\n",
           minor_faults() - f0, ms_since(&t0));
    f0 = minor_faults();
    for (size_t off = 0; off < SIZE; off += page)
      buf[off + 1] = 'c';
    printf("child:  wrote them again: %6ld minor faults (the copies are private now)\n",
           minor_faults() - f0);
    exit(sum == 0);
  }
  waitpid(pid, NULL, 0);
  printf("parent: buf[0] is still '%c'\n", buf[0]);
  return 0;
}
