#include "check.h"
#include "kernel.h"

#include <stdint.h>

/* User address space for these tests:
 *
 *   0x10000 .. 0x12fff  RW  three adjacent writable pages
 *   0x13000             --  unmapped
 *   0x14000             RO  read-only
 *   0x15000             RW  (holds iovec arrays in the writev test)
 *   USER_BREAK - PAGE_SIZE  RW  the last user page; the kernel follows
 */
#define U0      0x10000UL
#define U1      0x11000UL
#define U2      0x12000UL
#define HOLE    0x13000UL
#define RO      0x14000UL
#define IOVP    0x15000UL
#define LAST    (USER_BREAK - PAGE_SIZE)
#define KTEXT   0xffffffff80000000UL

static void setup(void)
{
  machine_reset();
  machine_map(U0, 1);
  machine_map(U1, 1);
  machine_map(U2, 1);
  machine_map(RO, 0);
  machine_map(IOVP, 1);
  machine_map(LAST, 1);
}

/* What the user-space side of SWEB's __syscall does: registers, trap. */
static long sys(size_t nr, size_t a1, size_t a2, size_t a3)
{
  struct regs r = {nr, a1, a2, a3, 0xdeadbeef, 0xdeadbeef};
  syscall_entry(&r);
  return (long)r.rax;
}

static void put(size_t uaddr, const char *s)
{
  machine_poke(uaddr, s, strlen(s) + 1);
}

static const char *console(void)
{
  return machine_console(NULL);
}

static size_t console_len(void)
{
  size_t n;
  machine_console(&n);
  return n;
}

#define CHECK_NO_KERNEL_LOOKUPS()                                               \
  CHECK(machine_kernel_lookups == 0,                                            \
        "the kernel looked up %zu kernel page(s) on behalf of a user pointer",  \
        machine_kernel_lookups)

#define CHECK_NO_LEAK()                                                         \
  CHECK(strstr(console(), "KERNEL-SECRET") == NULL,                             \
        "kernel memory ended up on the console!")

/* A write that runs into bad memory part-way may be short (kernel.h). */
static void check_short_write(long r, size_t ubuf, size_t n)
{
  if (r == -EFAULT) {
    CHECK(console_len() == 0, "write returned -EFAULT but the console got %zu bytes",
          console_len());
    return;
  }
  CHECK(r >= 0 && (size_t)r < n, "write(%#zx, %#zx) returned %ld: expected -EFAULT "
        "or a short count", ubuf, n, r);
  CHECK(console_len() == (size_t)(r < 0 ? 0 : r), "write returned %ld but the console "
        "got %zu bytes", r, console_len());
  if (r > 0) {
    char *want = malloc((size_t)r);
    machine_peek(ubuf, want, (size_t)r);
    CHECK(memcmp(console(), want, (size_t)r) == 0, "console contents differ from the buffer");
    free(want);
  }
}

/* -------------------------------- write -------------------------------- */
static void test_write(void)
{
  setup();
  put(U0, "hello, kernel\n");
  CHECK_EQ(sys(SC_WRITE, 1, U0, 14), 14);
  CHECK_STR(console(), "hello, kernel\n");
  CHECK_EQ(sys(SC_WRITE, 1, U0, 0), 0);
  CHECK_EQ(sys(SC_WRITE, 1, RO, 5), 5);                 /* reading a RO page is fine */
  CHECK_EQ(console_len(), 19);
  CHECK_NO_KERNEL_LOOKUPS();
}

static void test_write_three_pages(void)
{
  setup();
  size_t n = 3 * PAGE_SIZE - 100;
  char *data = malloc(n);
  for (size_t i = 0; i < n; i++)
    data[i] = (char)('a' + i % 26);
  machine_poke(U0 + 50, data, n);
  CHECK_EQ(sys(SC_WRITE, 1, U0 + 50, n), (long)n);
  CHECK(console_len() == n && memcmp(console(), data, n) == 0,
        "console does not contain the %zu bytes written", n);
  free(data);
}

static void test_write_bad_fd(void)
{
  setup();
  put(U0, "x");
  size_t fds[] = {0, 2, 3, 9, 1000, (size_t)-1, SIZE_MAX / 2};
  for (size_t i = 0; i < sizeof fds / sizeof fds[0]; i++)
    CHECK(sys(SC_WRITE, fds[i], U0, 1) == -EBADF, "write to fd %#zx should be -EBADF", fds[i]);
  CHECK_EQ(console_len(), 0);
}

static void test_write_bad_pointers(void)
{
  setup();
  CHECK_EQ(sys(SC_WRITE, 1, 0, 5), -EFAULT);
  CHECK_EQ(sys(SC_WRITE, 1, HOLE, 5), -EFAULT);
  CHECK_EQ(sys(SC_WRITE, 1, KTEXT, 64), -EFAULT);
  CHECK_EQ(sys(SC_WRITE, 1, USER_BREAK, 64), -EFAULT);
  CHECK_EQ(sys(SC_WRITE, 1, USER_BREAK - 8, 64), -EFAULT);
  /* LAST + n wraps around to 0x10 */
  CHECK_EQ(sys(SC_WRITE, 1, LAST, SIZE_MAX - LAST + 0x11), -EFAULT);
  CHECK_EQ(console_len(), 0);
  CHECK_NO_LEAK();
  CHECK_NO_KERNEL_LOOKUPS();
}

static void test_write_short(void)
{
  /* valid range, but it runs from mapped into unmapped pages */
  setup();
  check_short_write(sys(SC_WRITE, 1, U2 + 4000, 200), U2 + 4000, 200);
  setup();
  check_short_write(sys(SC_WRITE, 1, U0, 5 * PAGE_SIZE), U0, 5 * PAGE_SIZE);
  /* 128 TiB: must neither allocate that much nor take forever */
  setup();
  long long t0 = now_ms();
  check_short_write(sys(SC_WRITE, 1, U0, USER_BREAK - U0), U0, USER_BREAK - U0);
  CHECK(now_ms() - t0 < 2000, "a write of 128 TiB took %lld ms", now_ms() - t0);
  CHECK_NO_KERNEL_LOOKUPS();
}

/* -------------------------------- read --------------------------------- */
static void test_read_keyboard(void)
{
  setup();
  machine_keyboard("hello\n");
  CHECK_EQ(sys(SC_READ, 0, U1 - 3, 100), 6);            /* across U0|U1 */
  char buf[8] = {0};
  machine_peek(U1 - 3, buf, 6);
  CHECK_STR(buf, "hello\n");
  CHECK_EQ(sys(SC_READ, 0, U0, 100), 0);                /* nothing pending */

  machine_keyboard("abcdef");
  CHECK_EQ(sys(SC_READ, 0, U0, 4), 4);
  CHECK_EQ(sys(SC_READ, 0, U0 + 4, 4), 2);
  memset(buf, 0, sizeof buf);
  machine_peek(U0, buf, 6);
  CHECK_STR(buf, "abcdef");
  CHECK_NO_KERNEL_LOOKUPS();
}

static void test_read_errors(void)
{
  setup();
  machine_keyboard("typed text");
  CHECK_EQ(sys(SC_READ, 1, U0, 10), -EBADF);
  CHECK_EQ(sys(SC_READ, 2, U0, 10), -EBADF);
  CHECK_EQ(sys(SC_READ, (size_t)-1, U0, 10), -EBADF);
  CHECK_EQ(sys(SC_READ, 3, U0, 10), -EBADF);            /* no file open */

  CHECK_EQ(sys(SC_READ, 0, RO, 10), -EFAULT);
  char ro[10];
  machine_peek(RO, ro, sizeof ro);
  CHECK(memcmp(ro, "\0\0\0\0\0\0\0\0\0\0", 10) == 0, "the read-only page was modified");
  machine_keyboard("more");            /* the failed read above used up the input */
  CHECK_EQ(sys(SC_READ, 0, HOLE, 10), -EFAULT);
  CHECK_EQ(sys(SC_READ, 0, KTEXT, 10), -EFAULT);
  CHECK_EQ(sys(SC_READ, 0, USER_BREAK - 4, 10), -EFAULT);
  char k[8];
  machine_peek(USER_BREAK, k, sizeof k);
  CHECK(memcmp(k, machine_secret, sizeof k) == 0, "kernel memory was overwritten!");
  CHECK_NO_KERNEL_LOOKUPS();
}

/* ---------------------------- open / close ----------------------------- */
static void test_open_read_close(void)
{
  setup();
  machine_add_file("/etc/motd", "Welcome to SWEB\n");
  put(U0, "/etc/motd");
  long fd = sys(SC_OPEN, U0, 0, 0);
  CHECK(fd >= 3 && fd <= 9, "open returned %ld", fd);

  CHECK_EQ(sys(SC_READ, fd, U1, 4), 4);                 /* "Welc" */
  CHECK_EQ(sys(SC_READ, fd, U1 + 4, 100), 12);          /* "ome to SWEB\n" */
  CHECK_EQ(sys(SC_READ, fd, U1 + 16, 100), 0);          /* end of file */
  char buf[17] = {0};
  machine_peek(U1, buf, 16);
  CHECK_STR(buf, "Welcome to SWEB\n");

  CHECK_EQ(sys(SC_CLOSE, fd, 0, 0), 0);
  CHECK_EQ(sys(SC_CLOSE, fd, 0, 0), -EBADF);
  CHECK_EQ(sys(SC_READ, fd, U1, 4), -EBADF);
  CHECK_EQ(sys(SC_CLOSE, 0, 0, 0), -EBADF);
  CHECK_EQ(sys(SC_CLOSE, 1, 0, 0), -EBADF);
  CHECK_EQ(sys(SC_CLOSE, (size_t)-1, 0, 0), -EBADF);
  CHECK_NO_KERNEL_LOOKUPS();
}

static void test_open_errors(void)
{
  setup();
  machine_add_file("/etc/motd", "hi\n");
  put(U0, "/etc/passwd");
  CHECK_EQ(sys(SC_OPEN, U0, 0, 0), -ENOENT);
  CHECK_EQ(sys(SC_OPEN, 0, 0, 0), -EFAULT);
  CHECK_EQ(sys(SC_OPEN, HOLE, 0, 0), -EFAULT);
  CHECK_EQ(sys(SC_OPEN, KTEXT, 0, 0), -EFAULT);

  /* "/etc/motd" with its NUL missing: the next byte is unmapped */
  machine_poke(HOLE - 9, "/etc/motd", 9);
  CHECK_EQ(sys(SC_OPEN, HOLE - 9, 0, 0), -EFAULT);

  /* ... or is kernel memory */
  machine_poke(USER_BREAK - 9, "/etc/motd", 9);
  CHECK_EQ(sys(SC_OPEN, USER_BREAK - 9, 0, 0), -EFAULT);
  CHECK_NO_KERNEL_LOOKUPS();

  /* 63 characters are fine, 64 are too long */
  char name[KPATH_MAX + 1];
  memset(name, 'n', KPATH_MAX);
  name[0] = '/';
  name[KPATH_MAX - 1] = '\0';
  machine_add_file(name, "long name\n");
  put(U1, name);
  long fd = sys(SC_OPEN, U1, 0, 0);
  CHECK(fd >= 3, "open of a 63-character path returned %ld", fd);
  name[KPATH_MAX - 1] = 'n';
  name[KPATH_MAX] = '\0';
  put(U1, name);
  CHECK_EQ(sys(SC_OPEN, U1, 0, 0), -ENAMETOOLONG);
}

static void test_unknown_syscalls(void)
{
  setup();
  size_t nrs[] = {0, 1, 2, 7, 19, 21, 1000, (size_t)-1, (size_t)-4, SIZE_MAX / 2 + 3};
  for (size_t i = 0; i < sizeof nrs / sizeof nrs[0]; i++)
    CHECK(sys(nrs[i], 1, U0, 1) == -ENOSYS, "syscall %#zx should be -ENOSYS", nrs[i]);
  CHECK_EQ(console_len(), 0);
}

/* ------------------------------ bonus: writev ---------------------------- */
static void put_iov(size_t uaddr, const struct uiovec *iov, size_t n)
{
  machine_poke(uaddr, iov, n * sizeof *iov);
}

static void test_writev(void)
{
  setup();
  if (sys(SC_WRITEV, 1, IOVP, 0) == -ENOSYS)
    SKIP("writev not implemented (bonus)");

  put(U0, "Hello");
  put(U1, ", ");
  put(U2 - 3, "world\n");                               /* across U1|U2 */
  struct uiovec iov[] = {{U0, 5}, {U1, 2}, {U2 - 3, 6}, {U0, 0}};
  put_iov(IOVP + 16, iov, 4);

  size_t before = machine_page_lookups(IOVP);
  CHECK_EQ(sys(SC_WRITEV, 1, IOVP + 16, 4), 13);
  CHECK_STR(console(), "Hello, world\n");

  /* The iovec array must be fetched once, with a single copy_from_user.
   * Compare with what one copy_from_user of the same array costs. */
  size_t by_writev = machine_page_lookups(IOVP) - before;
  struct uiovec tmp[4];
  before = machine_page_lookups(IOVP);
  copy_from_user(tmp, IOVP + 16, sizeof tmp);
  size_t by_copy = machine_page_lookups(IOVP) - before;
  CHECK(by_writev == by_copy, "writev looked at the iovec page %zu times, one "
        "copy_from_user of the array takes %zu - fetch the array exactly once",
        by_writev, by_copy);

  CHECK_EQ(sys(SC_WRITEV, 1, IOVP, 0), 0);
  CHECK_EQ(sys(SC_WRITEV, 2, IOVP + 16, 4), -EBADF);
  CHECK_NO_KERNEL_LOOKUPS();
}

static void test_writev_errors(void)
{
  setup();
  if (sys(SC_WRITEV, 1, IOVP, 0) == -ENOSYS)
    SKIP("writev not implemented (bonus)");

  put(U0, "first");
  struct uiovec good = {U0, 5};

  /* too many entries - including a count whose byte size overflows */
  CHECK_EQ(sys(SC_WRITEV, 1, IOVP, UIO_MAXIOV + 1), -EINVAL);
  CHECK_EQ(sys(SC_WRITEV, 1, IOVP, SIZE_MAX / sizeof(struct uiovec) + 2), -EINVAL);
  CHECK_EQ(sys(SC_WRITEV, 1, IOVP, (size_t)-1), -EINVAL);

  /* the array itself in bad memory */
  CHECK_EQ(sys(SC_WRITEV, 1, HOLE, 1), -EFAULT);
  CHECK_EQ(sys(SC_WRITEV, 1, KTEXT, 1), -EFAULT);
  put_iov(IOVP + PAGE_SIZE - sizeof good, &good, 1);   /* entry 2 would be unmapped */
  CHECK_EQ(sys(SC_WRITEV, 1, IOVP + PAGE_SIZE - sizeof good, 2), -EFAULT);
  CHECK_EQ(console_len(), 0);

  /* one bad entry after a good one: nothing may be written */
  struct uiovec bad[][2] = {
    {{U0, 5}, {KTEXT, 16}},
    {{U0, 5}, {USER_BREAK - 8, 16}},
    {{U0, 5}, {LAST, SIZE_MAX - LAST + 0x11}},          /* wraps around */
  };
  for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
    put_iov(IOVP, bad[i], 2);
    CHECK(sys(SC_WRITEV, 1, IOVP, 2) == -EFAULT, "bad entry %zu: expected -EFAULT", i);
    CHECK(console_len() == 0, "bad entry %zu: the good first entry was already written", i);
  }
  CHECK_NO_LEAK();
  CHECK_NO_KERNEL_LOOKUPS();
}

int main(void)
{
  RUN_TEST(test_write);
  RUN_TEST(test_write_three_pages);
  RUN_TEST(test_write_bad_fd);
  RUN_TEST(test_write_bad_pointers);
  RUN_TEST(test_write_short);
  RUN_TEST(test_read_keyboard);
  RUN_TEST(test_read_errors);
  RUN_TEST(test_open_read_close);
  RUN_TEST(test_open_errors);
  RUN_TEST(test_unknown_syscalls);
  RUN_TEST(test_writev);
  RUN_TEST(test_writev_errors);
  machine_reset();
  return test_summary();
}
