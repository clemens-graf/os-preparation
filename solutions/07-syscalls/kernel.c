/*
 * kernel.c - Module 07, parts B and C: reference solution.
 */
#include "kernel.h"

#include <errno.h>
#include <string.h>

#define KBUF_SIZE 256

/* ------------------------------ Part B ------------------------------ */

int access_ok(size_t uaddr, size_t len)
{
  /* Never compute uaddr + len: it can wrap around. Both subtractions
   * below are safe - the first comparison guarantees len <= USER_BREAK. */
  return len <= USER_BREAK && uaddr <= USER_BREAK - len;
}

/* Bytes from uaddr to the end of its page, at most len. */
static size_t in_page(size_t uaddr, size_t len)
{
  size_t left = PAGE_SIZE - uaddr % PAGE_SIZE;
  return left < len ? left : len;
}

long copy_from_user(void *dst, size_t usrc, size_t len)
{
  if (!access_ok(usrc, len))
    return -EFAULT;
  unsigned char *d = dst;
  while (len > 0) {
    size_t n = in_page(usrc, len);
    const unsigned char *s = mmu_translate(usrc, 0);
    if (!s)
      return -EFAULT;
    memcpy(d, s, n);
    d += n;
    usrc += n;
    len -= n;
  }
  return 0;
}

long copy_to_user(size_t udst, const void *src, size_t len)
{
  if (!access_ok(udst, len))
    return -EFAULT;

  /* Pass 1: every page mapped and writable? (udst + len cannot overflow
   * any more - access_ok said so.) */
  for (size_t a = udst, left = len; left > 0;) {
    size_t n = in_page(a, left);
    if (!mmu_translate(a, 1))
      return -EFAULT;
    a += n;
    left -= n;
  }

  /* Pass 2: copy. In a real kernel another thread could unmap a page
   * between the passes; Linux therefore copies optimistically and lets
   * the page-fault handler abort the copy (exception tables). */
  const unsigned char *s = src;
  while (len > 0) {
    size_t n = in_page(udst, len);
    memcpy(mmu_translate(udst, 1), s, n);
    s += n;
    udst += n;
    len -= n;
  }
  return 0;
}

long strncpy_from_user(char *dst, size_t usrc, size_t max)
{
  size_t i = 0;
  while (i < max) {
    /* Check each page as the string reaches it: the end is unknown. */
    if (!access_ok(usrc + i, 1))
      return -EFAULT;
    const unsigned char *p = mmu_translate(usrc + i, 0);
    if (!p)
      return -EFAULT;
    size_t n = in_page(usrc + i, max - i);
    for (size_t k = 0; k < n; k++, i++) {
      dst[i] = (char)p[k];
      if (p[k] == '\0')
        return (long)i;
    }
  }
  return -ENAMETOOLONG;
}

/* ------------------------------ Part C ------------------------------ */

static long sys_read(size_t fd, size_t ubuf, size_t n)
{
  if (fd != 0 && fd < 3)
    return -EBADF;
  /* Check the range BEFORE taking data from the device. */
  if (!access_ok(ubuf, n))
    return -EFAULT;

  char kbuf[KBUF_SIZE];
  size_t want = n < KBUF_SIZE ? n : KBUF_SIZE;       /* a short read is fine */
  long got = fd == 0 ? (long)keyboard_read(kbuf, want) : vfs_read(fd, kbuf, want);
  if (got <= 0)
    return got;
  if (copy_to_user(ubuf, kbuf, (size_t)got) != 0)
    return -EFAULT;
  return got;
}

static long sys_write(size_t fd, size_t ubuf, size_t n)
{
  if (fd != 1)
    return -EBADF;
  if (!access_ok(ubuf, n))
    return -EFAULT;

  char kbuf[KBUF_SIZE];
  size_t done = 0;
  while (done < n) {
    size_t chunk = n - done < KBUF_SIZE ? n - done : KBUF_SIZE;
    if (copy_from_user(kbuf, ubuf + done, chunk) != 0)
      return done ? (long)done : -EFAULT;          /* short write */
    console_write(kbuf, chunk);
    done += chunk;
  }
  return (long)done;                                /* n <= USER_BREAK: fits */
}

static long sys_open(size_t upath)
{
  /* Copy the path ONCE into kernel memory and use only that copy. */
  char kpath[KPATH_MAX];
  long r = strncpy_from_user(kpath, upath, sizeof kpath);
  if (r < 0)
    return r;
  return vfs_open(kpath);
}

static long sys_close(size_t fd)
{
  return vfs_close(fd);
}

static long sys_writev(size_t fd, size_t uiov, size_t iovcnt)
{
  if (fd != 1)
    return -EBADF;
  /* Bound the count BEFORE multiplying it: iovcnt * 16 can overflow. */
  if (iovcnt > UIO_MAXIOV)
    return -EINVAL;

  /* One fetch. From now on only kiov is used: another thread changing
   * the user's array after our checks cannot affect us (no double fetch). */
  struct uiovec kiov[UIO_MAXIOV];
  long r = copy_from_user(kiov, uiov, iovcnt * sizeof kiov[0]);
  if (r)
    return r;

  for (size_t i = 0; i < iovcnt; i++)
    if (!access_ok(kiov[i].base, kiov[i].len))
      return -EFAULT;

  long total = 0;                  /* at most 16 * 2^47: fits into a long */
  for (size_t i = 0; i < iovcnt; i++) {
    long w = sys_write(fd, kiov[i].base, kiov[i].len);
    if (w < 0)
      return total ? total : w;
    total += w;
    if ((size_t)w < kiov[i].len)
      break;                       /* short write: stop here */
  }
  return total;
}

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
