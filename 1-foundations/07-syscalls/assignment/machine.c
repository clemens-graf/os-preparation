/*
 * machine.c - the simulated hardware for parts B and C. Given; do not edit.
 * Nothing here is thread-safe: the tests for parts B and C use one thread.
 */
#include "machine.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char machine_secret[] = "KERNEL-SECRET:root-password=hunter2;";
size_t machine_kernel_lookups;

static void die(const char *msg)
{
  fprintf(stderr, "machine: %s\n", msg);
  abort();
}

/* ---------------------------- memory ---------------------------- */

#define MAX_PAGES 64

struct page {
  size_t vpn;               /* virtual page number = vaddr / PAGE_SIZE */
  int writable;
  unsigned char *frame;     /* PAGE_SIZE bytes, a heap block of its own */
  size_t lookups;           /* mmu_translate calls for this page */
};

static struct page upages[MAX_PAGES];
static int nupages;

/* Kernel pages: the two right above USER_BREAK and one "kernel text" page. */
#define NKPAGES 3
static const size_t kvpns[NKPAGES] = {
  USER_BREAK / PAGE_SIZE, USER_BREAK / PAGE_SIZE + 1, 0xffffffff80000000UL / PAGE_SIZE,
};
static unsigned char *kframes[NKPAGES];

static void fill_kernel_pages(void)
{
  size_t slen = strlen(machine_secret);
  for (int k = 0; k < NKPAGES; k++) {
    if (!kframes[k] && !(kframes[k] = malloc(PAGE_SIZE)))
      die("out of memory");
    for (size_t i = 0; i < PAGE_SIZE; i++)
      kframes[k][i] = (unsigned char)machine_secret[i % slen];
  }
}

static struct page *find_upage(size_t vaddr)
{
  for (int i = 0; i < nupages; i++)
    if (upages[i].vpn == vaddr / PAGE_SIZE)
      return &upages[i];
  return NULL;
}

/* Page lookup without permission checks and without counting. */
static unsigned char *frame_byte(size_t vaddr)
{
  if (vaddr >= USER_BREAK) {
    for (int k = 0; k < NKPAGES; k++)
      if (kvpns[k] == vaddr / PAGE_SIZE)
        return kframes[k] + vaddr % PAGE_SIZE;
    return NULL;
  }
  struct page *p = find_upage(vaddr);
  return p ? p->frame + vaddr % PAGE_SIZE : NULL;
}

unsigned char *mmu_translate(size_t vaddr, int for_write)
{
  if (!kframes[0])
    fill_kernel_pages();
  if (vaddr >= USER_BREAK) {
    machine_kernel_lookups++;
    return frame_byte(vaddr);
  }
  struct page *p = find_upage(vaddr);
  if (p)
    p->lookups++;
  if (!p || (for_write && !p->writable))
    return NULL;
  return p->frame + vaddr % PAGE_SIZE;
}

/* ---------------------------- devices ---------------------------- */

#define CONSOLE_LIMIT (16UL << 20)
static char *console;
static size_t console_len, console_cap;

void console_write(const char *kbuf, size_t n)
{
  if (n > CONSOLE_LIMIT || console_len + n > CONSOLE_LIMIT)
    die("more than 16 MiB written to the console - a runaway write length?");
  if (console_len + n + 1 > console_cap) {
    console_cap = (console_len + n + 1) * 2;
    if (!(console = realloc(console, console_cap)))
      die("out of memory");
  }
  memcpy(console + console_len, kbuf, n);
  console_len += n;
  console[console_len] = '\0';
}

static char keyboard[256];
static size_t kb_len, kb_pos;

size_t keyboard_read(char *kbuf, size_t n)
{
  size_t k = kb_len - kb_pos < n ? kb_len - kb_pos : n;
  memcpy(kbuf, keyboard + kb_pos, k);
  kb_pos += k;
  return k;
}

/* ---------------------------- file system ---------------------------- */

#define MAX_FILES 8
#define FIRST_FD  3
#define LAST_FD   9

static struct { char name[64]; char contents[256]; } files[MAX_FILES];
static int nfiles;
static struct { int used, file; size_t pos; } fds[LAST_FD + 1];

long vfs_open(const char *kpath)
{
  for (int f = 0; f < nfiles; f++) {
    if (strcmp(files[f].name, kpath) != 0)
      continue;
    for (int fd = FIRST_FD; fd <= LAST_FD; fd++) {
      if (!fds[fd].used) {
        fds[fd].used = 1;
        fds[fd].file = f;
        fds[fd].pos = 0;
        return fd;
      }
    }
    return -EMFILE;
  }
  return -ENOENT;
}

long vfs_read(size_t fd, char *kbuf, size_t n)
{
  if (fd < FIRST_FD || fd > LAST_FD || !fds[fd].used)
    return -EBADF;
  const char *c = files[fds[fd].file].contents;
  size_t left = strlen(c) - fds[fd].pos;
  size_t k = left < n ? left : n;
  memcpy(kbuf, c + fds[fd].pos, k);
  fds[fd].pos += k;
  return (long)k;
}

long vfs_close(size_t fd)
{
  if (fd < FIRST_FD || fd > LAST_FD || !fds[fd].used)
    return -EBADF;
  fds[fd].used = 0;
  return 0;
}

/* ---------------------------- test harness ---------------------------- */

void machine_reset(void)
{
  for (int i = 0; i < nupages; i++)
    free(upages[i].frame);
  nupages = 0;
  fill_kernel_pages();
  machine_kernel_lookups = 0;
  console_len = 0;
  if (console)
    console[0] = '\0';
  kb_len = kb_pos = 0;
  nfiles = 0;
  memset(fds, 0, sizeof fds);
}

void machine_map(size_t vaddr, int writable)
{
  if (vaddr >= USER_BREAK)
    die("machine_map: not a user address");
  struct page *p = find_upage(vaddr);
  if (!p) {
    if (nupages == MAX_PAGES)
      die("machine_map: too many pages");
    p = &upages[nupages++];
    p->vpn = vaddr / PAGE_SIZE;
    p->lookups = 0;
    if (!(p->frame = calloc(1, PAGE_SIZE)))
      die("out of memory");
  }
  p->writable = writable;
}

void machine_poke(size_t vaddr, const void *src, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    unsigned char *b = frame_byte(vaddr + i);
    if (!b)
      die("machine_poke: address not mapped (bug in the test)");
    *b = ((const unsigned char *)src)[i];
  }
}

void machine_peek(size_t vaddr, void *dst, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    unsigned char *b = frame_byte(vaddr + i);
    if (!b)
      die("machine_peek: address not mapped (bug in the test)");
    ((unsigned char *)dst)[i] = *b;
  }
}

size_t machine_page_lookups(size_t vaddr)
{
  struct page *p = find_upage(vaddr);
  return p ? p->lookups : 0;
}

const char *machine_console(size_t *len)
{
  if (len)
    *len = console_len;
  return console ? console : "";
}

void machine_keyboard(const char *input)
{
  size_t n = strlen(input);
  if (kb_len + n > sizeof keyboard)
    die("machine_keyboard: too much input");
  memcpy(keyboard + kb_len, input, n);
  kb_len += n;
}

void machine_add_file(const char *name, const char *contents)
{
  if (nfiles == MAX_FILES)
    die("machine_add_file: too many files");
  snprintf(files[nfiles].name, sizeof files[0].name, "%s", name);
  snprintf(files[nfiles].contents, sizeof files[0].contents, "%s", contents);
  nfiles++;
}
