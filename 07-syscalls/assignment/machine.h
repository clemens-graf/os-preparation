/*
 * machine.h - the simulated hardware for parts B and C. Given; do not edit.
 *
 * A tiny model of one process's virtual address space, as the KERNEL sees
 * it while it handles a system call - the same picture as in SWEB:
 *
 *   0x0000000000000000  +---------------------------+
 *                       | user space                |  a few pages mapped by
 *                       | (mostly unmapped)         |  the tests, read-only
 *                       |                           |  or writable
 *   USER_BREAK          +---------------------------+  0x0000800000000000
 *                       | kernel space              |  always mapped for the
 *                       | "KERNEL-SECRET ..."       |  kernel
 *                       +---------------------------+
 *
 * Real x86-64 has a non-canonical hole between the two halves. Here the
 * kernel starts right at USER_BREAK instead, as on 32-bit x86 (SWEB's
 * 32-bit USER_BREAK 0x80000000 is also where its kernel begins): a range
 * that runs over USER_BREAK really reaches kernel memory.
 *
 * Every page is a separate heap block, so a memcpy that runs over the end
 * of a page is caught by AddressSanitizer - just as it would fault or read
 * a random physical frame in a real kernel.
 */
#pragma once
#include <stddef.h>

#define PAGE_SIZE  4096UL
#define USER_BREAK 0x0000800000000000UL

/* ------------------ for the kernel (your code) ------------------ */

/*
 * The MMU as the kernel sees it: a pointer to the byte at virtual address
 * vaddr, or NULL if that page is not mapped - or, with for_write != 0, is
 * read-only. The pointer is only valid up to the end of vaddr's page;
 * the next virtual page can live anywhere in (simulated) physical memory.
 *
 * Kernel addresses (>= USER_BREAK) translate fine: the kernel runs in
 * supervisor mode and may read and write its own memory. Nothing in the
 * MMU stops it from doing so on behalf of a user pointer - that check is
 * YOUR job. (The machine counts such look-ups, the tests expect none.)
 */
unsigned char *mmu_translate(size_t vaddr, int for_write);

/* fd 1: append n bytes to the console. */
void console_write(const char *kbuf, size_t n);

/* fd 0: take up to n bytes of pending keyboard input; returns how many
 * (0 when no input is pending). */
size_t keyboard_read(char *kbuf, size_t n);

/* A read-only file system with its own open-file table; its file
 * descriptors are 3 ... 9. kpath must be a KERNEL string. */
long vfs_open(const char *kpath);                    /* fd, -ENOENT or -EMFILE */
long vfs_read(size_t fd, char *kbuf, size_t n);      /* bytes read or -EBADF */
long vfs_close(size_t fd);                           /* 0 or -EBADF */

/* ------------------ for the tests only ------------------ */

void machine_reset(void);                     /* unmap all user pages, clear devices */
void machine_map(size_t vaddr, int writable); /* map the (zeroed) page containing vaddr */
void machine_poke(size_t vaddr, const void *src, size_t n);  /* ignores permissions */
void machine_peek(size_t vaddr, void *dst, size_t n);
const char *machine_console(size_t *len);     /* everything written so far, NUL-terminated */
void machine_keyboard(const char *input);     /* queue keyboard input */
void machine_add_file(const char *name, const char *contents);

size_t machine_page_lookups(size_t vaddr);    /* mmu_translate calls for that user page */

extern const char machine_secret[];           /* the text filling every kernel page */
extern size_t machine_kernel_lookups;         /* mmu_translate calls with vaddr >= USER_BREAK */
