#include "check.h"
#include "kernel.h"

#include <stdint.h>

/* User address space used by most tests (all other pages are unmapped):
 *
 *   0x10000  RW  \ two adjacent writable pages
 *   0x11000  RW  /
 *   0x12000  --  unmapped
 *   0x13000  RO  read-only ("program code")
 *   0x14000  RW
 *   USER_BREAK - PAGE_SIZE  RW   the last user page; the kernel follows
 */
#define PG_A    0x10000UL
#define PG_B    0x11000UL
#define PG_HOLE 0x12000UL
#define PG_RO   0x13000UL
#define PG_C    0x14000UL
#define PG_LAST (USER_BREAK - PAGE_SIZE)
#define KTEXT   0xffffffff80000000UL

static void setup(void)
{
  machine_reset();
  machine_map(PG_A, 1);
  machine_map(PG_B, 1);
  machine_map(PG_RO, 0);
  machine_map(PG_C, 1);
  machine_map(PG_LAST, 1);
}

static void fill_pattern(size_t uaddr, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    unsigned char b = (unsigned char)(i * 7 + 1);
    machine_poke(uaddr + i, &b, 1);
  }
}

static int has_pattern(const unsigned char *buf, size_t n)
{
  for (size_t i = 0; i < n; i++)
    if (buf[i] != (unsigned char)(i * 7 + 1))
      return 0;
  return 1;
}

#define CHECK_NO_KERNEL_LOOKUPS()                                               \
  CHECK(machine_kernel_lookups == 0,                                            \
        "the kernel looked up %zu kernel page(s) on behalf of a user pointer",  \
        machine_kernel_lookups)

/* ------------------------------ access_ok ------------------------------ */
static void test_access_ok(void)
{
  static const struct { size_t addr, len; int ok; } cases[] = {
    {0x10000, 16, 1},
    {0, 1, 1},                                   /* NULL is a user address... */
    {USER_BREAK - 16, 16, 1},                    /* ends exactly at the break */
    {USER_BREAK - 1, 1, 1},
    {USER_BREAK - 16, 17, 0},                    /* one byte of kernel */
    {USER_BREAK, 1, 0},
    {USER_BREAK, 0, 1},                          /* empty range at the break */
    {USER_BREAK + 1, 0, 0},
    {KTEXT, 1, 0},
    {0, USER_BREAK, 1},                          /* all of user space */
    {0, USER_BREAK + 1, 0},
    {1, USER_BREAK, 0},
    {0x10000, SIZE_MAX, 0},                      /* 0x10000 + len wraps to 0xffff */
    {0x10000, SIZE_MAX - 0xffff + 0x10, 0},      /* wraps to 0x10: "below the break" */
    {USER_BREAK - 1, SIZE_MAX, 0},
    {SIZE_MAX, 1, 0},                            /* wraps to 0 */
    {SIZE_MAX - 3, 8, 0},
  };
  for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    int got = access_ok(cases[i].addr, cases[i].len) != 0;
    CHECK(got == cases[i].ok, "access_ok(%#zx, %#zx) = %d, expected %d",
          cases[i].addr, cases[i].len, got, cases[i].ok);
  }
}

/* ---------------------------- copy_from_user ---------------------------- */
static void test_copy_from_user_one_page(void)
{
  setup();
  fill_pattern(PG_A + 100, 200);
  unsigned char buf[200] = {0};
  CHECK_EQ(copy_from_user(buf, PG_A + 100, 200), 0);
  CHECK(has_pattern(buf, 200), "wrong bytes copied");
  CHECK_EQ(copy_from_user(buf, PG_A, 0), 0);
  CHECK_NO_KERNEL_LOOKUPS();
}

static void test_copy_from_user_across_pages(void)
{
  setup();
  size_t start = PG_B - 1000, n = 3000;          /* 1000 bytes in A, 2000 in B */
  fill_pattern(start, n);
  unsigned char *buf = calloc(1, n);
  CHECK_EQ(copy_from_user(buf, start, n), 0);
  CHECK(has_pattern(buf, n), "wrong bytes copied across the page boundary");

  /* A and B completely, exactly 2 pages */
  fill_pattern(PG_A, 2 * PAGE_SIZE);
  unsigned char *big = calloc(1, 2 * PAGE_SIZE);
  CHECK_EQ(copy_from_user(big, PG_A, 2 * PAGE_SIZE), 0);
  CHECK(has_pattern(big, 2 * PAGE_SIZE), "wrong bytes copied for two full pages");
  free(buf);
  free(big);
}

static void test_copy_from_user_faults(void)
{
  setup();
  unsigned char buf[64];
  CHECK_EQ(copy_from_user(buf, 0, 8), -EFAULT);                 /* NULL, unmapped */
  CHECK_EQ(copy_from_user(buf, PG_HOLE + 8, 8), -EFAULT);       /* unmapped */
  CHECK_EQ(copy_from_user(buf, PG_HOLE - 4, 8), -EFAULT);       /* runs into the hole */
  CHECK_EQ(copy_from_user(buf, PG_RO + 8, 8), 0);               /* read-only is readable */
  CHECK_NO_KERNEL_LOOKUPS();

  CHECK_EQ(copy_from_user(buf, KTEXT, 16), -EFAULT);            /* kernel memory */
  CHECK_EQ(copy_from_user(buf, USER_BREAK, 16), -EFAULT);
  CHECK_EQ(copy_from_user(buf, USER_BREAK - 8, 16), -EFAULT);   /* half user, half kernel */
  CHECK_NO_KERNEL_LOOKUPS();

  /* The overflow: PG_LAST + len wraps around to 0x10, "below the break".
   * A naive check lets it pass; the page walk then continues from the
   * last user page straight into the kernel. */
  unsigned char *big = calloc(4, PAGE_SIZE);
  CHECK_EQ(copy_from_user(big, PG_LAST, SIZE_MAX - PG_LAST + 0x11), -EFAULT);
  CHECK_NO_KERNEL_LOOKUPS();
  CHECK(memmem(big, 4 * PAGE_SIZE, "KERNEL-SECRET", 13) == NULL,
        "copy_from_user leaked kernel memory");
  free(big);
}

/* ----------------------------- copy_to_user ----------------------------- */
static void test_copy_to_user(void)
{
  setup();
  unsigned char src[3000];
  for (size_t i = 0; i < sizeof src; i++)
    src[i] = (unsigned char)(i * 7 + 1);

  CHECK_EQ(copy_to_user(PG_B - 1000, src, sizeof src), 0);      /* across A|B */
  unsigned char back[3000];
  machine_peek(PG_B - 1000, back, sizeof back);
  CHECK(has_pattern(back, sizeof back), "wrong bytes in user memory");
  CHECK_EQ(copy_to_user(PG_C, src, 0), 0);
  CHECK_NO_KERNEL_LOOKUPS();
}

static int all_zero(size_t uaddr, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    unsigned char b;
    machine_peek(uaddr + i, &b, 1);
    if (b)
      return 0;
  }
  return 1;
}

static void test_copy_to_user_faults(void)
{
  setup();
  unsigned char src[64];
  memset(src, 0xAA, sizeof src);

  CHECK_EQ(copy_to_user(PG_RO + 8, src, 8), -EFAULT);           /* read-only page */
  CHECK(all_zero(PG_RO, PAGE_SIZE), "the read-only page was modified");
  CHECK_EQ(copy_to_user(PG_HOLE, src, 8), -EFAULT);
  CHECK_EQ(copy_to_user(0, src, 8), -EFAULT);

  /* writable page followed by read-only / unmapped: all or nothing */
  machine_map(PG_RO - PAGE_SIZE, 1);                            /* 0x12000 now RW */
  CHECK_EQ(copy_to_user(PG_RO - 32, src, 64), -EFAULT);
  CHECK(all_zero(PG_RO - PAGE_SIZE, PAGE_SIZE),
        "copy_to_user failed but changed the writable page before the read-only one");
  CHECK_EQ(copy_to_user(PG_C + PAGE_SIZE - 32, src, 64), -EFAULT);   /* C, then unmapped */
  CHECK(all_zero(PG_C, PAGE_SIZE),
        "copy_to_user failed but changed the page before the unmapped one");
  CHECK_NO_KERNEL_LOOKUPS();

  /* into the kernel */
  CHECK_EQ(copy_to_user(KTEXT, src, 8), -EFAULT);
  CHECK_EQ(copy_to_user(PG_LAST + PAGE_SIZE - 8, src, 16), -EFAULT);
  CHECK(all_zero(PG_LAST, PAGE_SIZE), "last user page changed");
  char k[8];
  machine_peek(USER_BREAK, k, sizeof k);
  CHECK(memcmp(k, machine_secret, sizeof k) == 0, "kernel memory was overwritten!");
  CHECK_EQ(copy_to_user(PG_LAST, src, SIZE_MAX - PG_LAST + 0x11), -EFAULT);  /* wraps */
  CHECK(all_zero(PG_LAST, PAGE_SIZE), "last user page changed");
  CHECK_NO_KERNEL_LOOKUPS();
}

/* --------------------------- strncpy_from_user --------------------------- */
static void put_str(size_t uaddr, const char *s)
{
  machine_poke(uaddr, s, strlen(s) + 1);
}

static void test_strncpy_from_user(void)
{
  setup();
  char buf[65] = {0};                    /* max 64: buf[64] stays NUL */
  put_str(PG_A + 10, "/bin/sh");
  memset(buf, 'x', 64);
  CHECK_EQ(strncpy_from_user(buf, PG_A + 10, 64), 7);
  CHECK_STR(buf, "/bin/sh");

  put_str(PG_B - 3, "across");                                  /* "acr" | "oss\0" */
  CHECK_EQ(strncpy_from_user(buf, PG_B - 3, 64), 6);
  CHECK_STR(buf, "across");

  put_str(PG_C, "");
  CHECK_EQ(strncpy_from_user(buf, PG_C, 64), 0);
  CHECK_STR(buf, "");

  put_str(PG_RO + 100, "readonly is fine");
  CHECK_EQ(strncpy_from_user(buf, PG_RO + 100, 64), 16);
  CHECK_NO_KERNEL_LOOKUPS();
}

static void test_strncpy_from_user_limits(void)
{
  setup();
  char buf[16] = {0};                                           /* max 8 */
  put_str(PG_A, "1234567");                                     /* 7 + NUL: fits */
  CHECK_EQ(strncpy_from_user(buf, PG_A, 8), 7);
  CHECK_STR(buf, "1234567");
  put_str(PG_A, "12345678");                                    /* 8 + NUL: too long */
  CHECK_EQ(strncpy_from_user(buf, PG_A, 8), -ENAMETOOLONG);

  /* the page is full of 'x' and no NUL, followed by an unmapped page */
  char xs[PAGE_SIZE];
  memset(xs, 'x', sizeof xs);
  machine_poke(PG_C, xs, sizeof xs);
  char big[2 * PAGE_SIZE];
  CHECK_EQ(strncpy_from_user(big, PG_C + PAGE_SIZE - 10, sizeof big), -EFAULT);
  CHECK_EQ(strncpy_from_user(big, PG_C + PAGE_SIZE - 10, 10), -ENAMETOOLONG);
  CHECK_EQ(strncpy_from_user(big, PG_C + PAGE_SIZE - 10, 11), -EFAULT);
  CHECK_EQ(strncpy_from_user(big, PG_HOLE, sizeof big), -EFAULT);
  CHECK_EQ(strncpy_from_user(big, 0, sizeof big), -EFAULT);
  CHECK_NO_KERNEL_LOOKUPS();
}

static void test_strncpy_from_user_kernel(void)
{
  setup();
  char buf[65] = {0};                    /* max 64: buf[64] stays NUL */
  CHECK_EQ(strncpy_from_user(buf, KTEXT, 64), -EFAULT);
  CHECK_EQ(strncpy_from_user(buf, USER_BREAK, 64), -EFAULT);

  /* a string at the very end of user space without a NUL: continuing
   * would read "KERNEL-SECRET..." from the first kernel page */
  char xs[PAGE_SIZE];
  memset(xs, 'x', sizeof xs);
  machine_poke(PG_LAST, xs, sizeof xs);
  memset(buf, 0, sizeof buf);
  long r = strncpy_from_user(buf, USER_BREAK - 5, 64);
  CHECK(r == -EFAULT, "string running into the kernel: got %ld, expected -EFAULT (%d)%s%.*s",
        r, -EFAULT, r > 0 ? ", copied: " : "", r > 0 ? (int)r : 0, buf);
  CHECK_NO_KERNEL_LOOKUPS();
}

int main(void)
{
  RUN_TEST(test_access_ok);
  RUN_TEST(test_copy_from_user_one_page);
  RUN_TEST(test_copy_from_user_across_pages);
  RUN_TEST(test_copy_from_user_faults);
  RUN_TEST(test_copy_to_user);
  RUN_TEST(test_copy_to_user_faults);
  RUN_TEST(test_strncpy_from_user);
  RUN_TEST(test_strncpy_from_user_limits);
  RUN_TEST(test_strncpy_from_user_kernel);
  machine_reset();
  return test_summary();
}
