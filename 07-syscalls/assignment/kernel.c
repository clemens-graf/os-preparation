/*
 * kernel.c - Module 07 assignment, parts B and C. Replace the TODOs.
 * Test:  make test-uaccess  (part B)   make test-syscalls  (part C)
 *
 * All user memory goes through mmu_translate() (machine.h), which only
 * vouches for ONE page at a time.
 */
#include "kernel.h"

#include <errno.h>
#include <string.h>

/* Kernel stacks are small (a few KiB in SWEB and Linux): move data between
 * user space and devices through a buffer of this size, in chunks. */
#define KBUF_SIZE 256

/* ------------------------------ Part B ------------------------------ */

int access_ok(size_t uaddr, size_t len)
{
  /* This is the check from SWEB's Syscall::write. It has a hole:
   * find inputs that pass it although the range reaches kernel memory
   * (questions.md, Q2), then TODO: rewrite it so it cannot overflow. */
  return !((uaddr >= USER_BREAK) || (uaddr + len > USER_BREAK));
}

long copy_from_user(void *dst, size_t usrc, size_t len)
{
  /* TODO: access_ok first; then page by page - mmu_translate() the
   * current address, copy up to the end of that page (or less), move on.
   * (A single memcpy(dst, mmu_translate(usrc, 0), len) is wrong as soon as
   * the range crosses a page boundary - AddressSanitizer will show you.) */
  return -ENOSYS;
}

long copy_to_user(size_t udst, const void *src, size_t len)
{
  /* TODO: like copy_from_user, with for_write = 1. All or nothing: make
   * sure every page is writable before changing the first byte. */
  return -ENOSYS;
}

long strncpy_from_user(char *dst, size_t usrc, size_t max)
{
  /* TODO: copy byte by byte (or page by page) until the NUL; every byte
   * address must be checked - the string may run into an unmapped page
   * or over USER_BREAK. */
  return -ENOSYS;
}

/* ------------------------------ Part C ------------------------------ */

static long sys_read(size_t fd, size_t ubuf, size_t n)
{
  /* TODO: fd 0 -> keyboard_read, fds 3..9 -> vfs_read (it rejects bad
   * fds itself), anything else -> -EBADF. Through a KBUF_SIZE buffer and
   * copy_to_user; one chunk per call is fine (a short read). */
  return -ENOSYS;
}

static long sys_write(size_t fd, size_t ubuf, size_t n)
{
  /* TODO: only fd 1. Check the whole range with access_ok first, then
   * copy_from_user + console_write in KBUF_SIZE chunks. */
  return -ENOSYS;
}

static long sys_open(size_t upath)
{
  /* TODO: strncpy_from_user into a KPATH_MAX buffer, then vfs_open. */
  return -ENOSYS;
}

static long sys_close(size_t fd)
{
  /* TODO */
  return -ENOSYS;
}

static long sys_writev(size_t fd, size_t uiov, size_t iovcnt)
{
  /* Bonus. TODO: see kernel.h. */
  return -ENOSYS;
}

/* Given: the dispatcher, like Syscall::syscallException in SWEB. Note that
 * r->rax may be ANY number - a table of function pointers indexed by it
 * would need a bounds check. */
void syscall_entry(struct regs *r)
{
  long ret;
  switch (r->rax) {
  case SC_READ:   ret = sys_read(r->rbx, r->rcx, r->rdx);   break;
  case SC_WRITE:  ret = sys_write(r->rbx, r->rcx, r->rdx);  break;
  case SC_OPEN:   ret = sys_open(r->rbx);                   break;
  case SC_CLOSE:  ret = sys_close(r->rbx);                  break;
  case SC_WRITEV: ret = sys_writev(r->rbx, r->rcx, r->rdx); break;
  default:        ret = -ENOSYS;
  }
  r->rax = (size_t)ret;
}
