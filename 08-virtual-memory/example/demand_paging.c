/*
 * demand_paging.c - memory is promised first and delivered on first touch.
 *
 * mmap() of 1 GiB only creates a REGION in the process's address space
 * (like as_add_region in the assignment); no frame is allocated and no
 * page-table entry exists yet. The first access to each page raises a
 * page fault, the kernel allocates a zeroed frame and maps it: a "minor"
 * fault (no disk involved). RSS - resident set size - counts the frames
 * the process really has.
 *
 * Reading an untouched page faults too, but Linux then maps one shared,
 * read-only page full of zeros: RSS does not grow until the first write
 * (which is a copy-on-write fault on the zero page).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

static long minor_faults(void)
{
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  return ru.ru_minflt;
}

static long rss_kib(void)
{
  long pages = 0, resident = 0;
  FILE *f = fopen("/proc/self/statm", "r");
  if (f) {
    if (fscanf(f, "%ld %ld", &pages, &resident) != 2)
      resident = 0;
    fclose(f);
  }
  return resident * (sysconf(_SC_PAGESIZE) / 1024);
}

static void report(const char *when, long faults0)
{
  printf("  %-34s RSS %8ld KiB, minor faults so far %6ld\n", when, rss_kib(),
         minor_faults() - faults0);
}

int main(void)
{
  const size_t GiB = 1UL << 30, page = (size_t)sysconf(_SC_PAGESIZE);
  long f0 = minor_faults();
  report("start", f0);

  char *p = mmap(NULL, GiB, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED) {
    perror("mmap");
    return 1;
  }
  report("after mmap(1 GiB)", f0);

  for (size_t off = 0; off < 100 * (1UL << 20); off += page)
    p[off] = 1;                       /* one byte per page: one fault per page */
  report("after touching 100 MiB", f0);

  long zero = 0;
  for (size_t off = 200 * (1UL << 20); off < 210 * (1UL << 20); off += page)
    zero += p[off] != 0;              /* reading also faults - and sees zeros */
  report("after reading 10 more MiB", f0);   /* faults, but the shared zero page */
  printf("  non-zero bytes seen in fresh pages: %ld\n", zero);

  munmap(p, GiB);
  report("after munmap", f0);

  printf("\nThe address space (from /proc/self/maps), abbreviated:\n");
  FILE *m = fopen("/proc/self/maps", "r");
  char line[512];
  while (m && fgets(line, sizeof line, m))
    if (strstr(line, "demand_paging") || strstr(line, "[heap]") || strstr(line, "[stack]") ||
        strstr(line, "libc.so"))
      printf("  %s", line);
  if (m)
    fclose(m);
  return 0;
}
