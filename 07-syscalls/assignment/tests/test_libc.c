#include "check.h"
#include "mini_libc.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>

static char tmpdir[] = "/tmp/mini_libc_XXXXXX";

static void make_file(const char *path, const void *data, size_t n)
{
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0 || write(fd, data, n) != (ssize_t)n) {
    perror(path);
    exit(2);
  }
  close(fd);
}

/* ------------------------------------------------------------------ */
static void test_raw_syscall(void)
{
  CHECK_EQ(ml_syscall6(SYS_getppid, 0, 0, 0, 0, 0, 0), getppid());
  CHECK_EQ(ml_syscall6(SYS_write, 999, (long)"x", 1, 0, 0, 0), -EBADF);
  CHECK_EQ(ml_syscall6(99999, 0, 0, 0, 0, 0, 0), -ENOSYS);
  /* six arguments: mmap(NULL, 4096, RW, PRIVATE|ANON, -1, 0) */
  long p = ml_syscall6(SYS_mmap, 0, 4096, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  CHECK(p > 0 && p % 4096 == 0, "raw mmap returned %ld (%s)", p,
        p < 0 && p > -4096 ? strerror((int)-p) : "?");
  if (p > 0)
    munmap((void *)p, 4096);
}

static void test_getpid(void)
{
  CHECK_EQ(ml_getpid(), getpid());
}

static void test_write_read_pipe(void)
{
  int p[2];                        /* non-blocking: an empty pipe must not hang the test */
  if (pipe2(p, O_NONBLOCK) != 0) { perror("pipe2"); exit(2); }

  CHECK_EQ(ml_write(p[1], "hello syscall", 13), 13);
  char buf[32] = {0};
  CHECK_EQ(read(p[0], buf, sizeof buf), 13);
  CHECK_STR(buf, "hello syscall");

  CHECK_EQ(write(p[1], "back", 4), 4);
  memset(buf, 0, sizeof buf);
  CHECK_EQ(ml_read(p[0], buf, sizeof buf), 4);
  CHECK_STR(buf, "back");

  CHECK_EQ(ml_write(p[1], "", 0), 0);
  close(p[0]);
  close(p[1]);
}

static void test_errors_set_ml_errno(void)
{
  ml_errno = 0;
  CHECK_EQ(ml_write(12345, "x", 1), -1);
  CHECK_EQ(ml_errno, EBADF);

  int p[2];
  if (pipe(p) != 0) { perror("pipe"); exit(2); }
  ml_errno = 0;
  CHECK_EQ(ml_write(p[1], (void *)1, 5), -1);          /* the kernel checks pointers */
  CHECK_EQ(ml_errno, EFAULT);
  ml_errno = 0;
  CHECK_EQ(ml_read(p[1], (char[4]){0}, 4), -1);        /* write end is not readable */
  CHECK_EQ(ml_errno, EBADF);

  ml_errno = 1234;                                     /* success leaves it alone */
  CHECK_EQ(ml_write(p[1], "ok", 2), 2);
  CHECK_EQ(ml_errno, 1234);
  close(p[0]);
  close(p[1]);
}

static void test_open_read_close(void)
{
  char path[64];
  snprintf(path, sizeof path, "%s/data.txt", tmpdir);
  make_file(path, "file contents\n", 14);

  int fd = ml_open(path, O_RDONLY, 0);
  CHECK(fd >= 0, "ml_open(\"%s\") returned %d, ml_errno %d", path, fd, ml_errno);
  if (fd >= 0) {
    char buf[32] = {0};
    CHECK_EQ(ml_read(fd, buf, sizeof buf), 14);
    CHECK_STR(buf, "file contents\n");
    CHECK_EQ(ml_read(fd, buf, sizeof buf), 0);         /* end of file */
    CHECK_EQ(ml_close(fd), 0);
    ml_errno = 0;
    CHECK_EQ(ml_close(fd), -1);                        /* already closed */
    CHECK_EQ(ml_errno, EBADF);
  }

  ml_errno = 0;
  CHECK_EQ(ml_open("/nonexistent/file", O_RDONLY, 0), -1);
  CHECK_EQ(ml_errno, ENOENT);
}

/* The mode is the 3rd argument of SYS_open but the 4th of SYS_openat -
 * with openat it travels in r10. */
static void test_open_create_mode(void)
{
  char path[64];
  snprintf(path, sizeof path, "%s/created.txt", tmpdir);
  mode_t old = umask(022);
  int fd = ml_open(path, O_WRONLY | O_CREAT | O_EXCL, 0640);
  umask(old);
  CHECK(fd >= 0, "ml_open(O_CREAT) returned %d, ml_errno %d", fd, ml_errno);
  if (fd >= 0) {
    struct stat st;
    CHECK(fstat(fd, &st) == 0, "fstat failed");
    CHECK((st.st_mode & 0777) == 0640, "file created with mode %o, expected 640",
          st.st_mode & 0777);
    ml_close(fd);
  }
}

static void test_mmap(void)
{
  /* anonymous, two pages */
  char *p = ml_mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  CHECK(p != MAP_FAILED, "anonymous ml_mmap failed, ml_errno %d", ml_errno);
  if (p != MAP_FAILED) {
    CHECK(p[0] == 0 && p[8191] == 0, "fresh anonymous memory must be zero");
    memset(p, 'x', 8192);
    CHECK_EQ(ml_munmap(p, 8192), 0);
  }

  /* file-backed, at an offset: needs fd (r8) and offset (r9) */
  char path[64], data[8192];
  snprintf(path, sizeof path, "%s/pages.bin", tmpdir);
  memset(data, 'A', 4096);
  memset(data + 4096, 'B', 4096);
  make_file(path, data, sizeof data);
  int fd = open(path, O_RDONLY);
  char *q = ml_mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 4096);
  CHECK(q != MAP_FAILED, "file ml_mmap failed, ml_errno %d", ml_errno);
  if (q != MAP_FAILED) {
    CHECK(q[0] == 'B' && q[4095] == 'B', "offset 4096 should map the 'B' page, got '%c'", q[0]);
    ml_munmap(q, 4096);
  }

  ml_errno = 0;
  CHECK(ml_mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 100) == MAP_FAILED,
        "unaligned offset must fail");
  CHECK_EQ(ml_errno, EINVAL);
  close(fd);

  ml_errno = 0;
  CHECK(ml_mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, 999, 0) == MAP_FAILED,
        "bad fd must fail");
  CHECK_EQ(ml_errno, EBADF);

  ml_errno = 0;
  CHECK(ml_mmap(NULL, 4096, PROT_READ, MAP_ANONYMOUS, -1, 0) == MAP_FAILED,
        "neither MAP_PRIVATE nor MAP_SHARED must fail");
  CHECK_EQ(ml_errno, EINVAL);
}

/* ---- errno is per thread ---- */
static pthread_barrier_t bar;

static void *fail_badf(void *unused)
{
  ml_write(12345, "x", 1);
  pthread_barrier_wait(&bar);             /* both threads have failed now */
  return (void *)(long)ml_errno;
}

static void *fail_noent(void *unused)
{
  ml_open("/nonexistent/file", O_RDONLY, 0);
  pthread_barrier_wait(&bar);
  return (void *)(long)ml_errno;
}

static void test_errno_per_thread(void)
{
  pthread_barrier_init(&bar, NULL, 2);
  pthread_t a, b;
  void *ra, *rb;
  pthread_create(&a, NULL, fail_badf, NULL);
  pthread_create(&b, NULL, fail_noent, NULL);
  pthread_join(a, &ra);
  pthread_join(b, &rb);
  CHECK_EQ((long)ra, EBADF);
  CHECK_EQ((long)rb, ENOENT);
  pthread_barrier_destroy(&bar);
}

/* ---- exit ends the whole process ---- */
static void *sleep_forever(void *unused)
{
  for (;;)
    pause();
  return NULL;
}

static void *exit_from_thread(void *unused)
{
  ml_exit(7);
}

/* Returns the child's wait status, or -1 if it is still alive after 3 s. */
static int wait_child(pid_t pid)
{
  int status;
  for (int i = 0; i < 300; i++) {
    if (waitpid(pid, &status, WNOHANG) == pid)
      return status;
    sleep_ms(10);
  }
  kill(pid, SIGKILL);
  waitpid(pid, &status, 0);
  return -1;
}

static void check_exit_status(int status, int expected)
{
  if (status == -1)
    CHECK(0, "process still running 3 s after ml_exit(%d): only one thread "
             "ended - SYS_exit instead of SYS_exit_group?", expected);
  else if (WIFSIGNALED(status))
    CHECK(0, "child killed by signal %d (%s) instead of exiting with %d",
          WTERMSIG(status), strsignal(WTERMSIG(status)), expected);
  else
    CHECK_EQ(WEXITSTATUS(status), expected);
}

static void test_exit(void)
{
  fflush(NULL);
  pid_t pid = fork();
  if (pid == 0)
    ml_exit(42);
  check_exit_status(wait_child(pid), 42);

  /* main thread exits while another thread is still running */
  pid = fork();
  if (pid == 0) {
    pthread_t t;
    pthread_create(&t, NULL, sleep_forever, NULL);
    ml_exit(3);
  }
  check_exit_status(wait_child(pid), 3);

  /* a second thread exits while main is still running */
  pid = fork();
  if (pid == 0) {
    pthread_t t;
    pthread_create(&t, NULL, exit_from_thread, NULL);
    sleep_forever(NULL);
  }
  check_exit_status(wait_child(pid), 7);
}

int main(void)
{
  if (!mkdtemp(tmpdir)) { perror("mkdtemp"); return 2; }

  RUN_TEST(test_raw_syscall);
  RUN_TEST(test_getpid);
  RUN_TEST(test_write_read_pipe);
  RUN_TEST(test_errors_set_ml_errno);
  RUN_TEST(test_open_read_close);
  RUN_TEST(test_open_create_mode);
  RUN_TEST(test_mmap);
  RUN_TEST(test_errno_per_thread);
  RUN_TEST(test_exit);

  char cmd[128];
  snprintf(cmd, sizeof cmd, "rm -rf %s", tmpdir);
  if (system(cmd) != 0) {}
  return test_summary();
}
