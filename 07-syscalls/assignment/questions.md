# Module 07 — Paper questions

Answers: `../../solutions/07-syscalls/answers.md`.
Paths refer to your SWEB repo (`repos/osw26e3`).

**Q1.** A user program knows the address of `Syscall::write` in the SWEB
kernel. Why can it not simply `call` that address? And why does
`int $0x80` get it into the kernel, while `int $0x0e` (the page-fault
vector) does not? (Look for `dpl` in
`arch/x86/64/source/InterruptUtils.cpp`.)

**Q2.** `Syscall::write` checks
`(buffer >= USER_BREAK) || (buffer + size > USER_BREAK)`.
Give concrete values for `buffer` and `size` that pass this check
although the range reaches beyond `USER_BREAK`. What does the fd-1 branch
do next with them (look at the `%.*s` and `(int)size`)? Write a correct
check.

**Q3.** `Syscall::open` checks only `path >= USER_BREAK`. What can go
wrong? Why can the kernel not fix it with
`path + strlen(path) >= USER_BREAK`?

**Q4.** A kernel implements `writev` like this, with `iov` pointing into
user memory:
```c
for (i = 0; i < iovcnt; i++) {
  if (!access_ok(iov[i].base, iov[i].len)) return -EFAULT;
  console_write((char *)iov[i].base, iov[i].len);
}
```
Describe an attack with two threads of the same process. What is this
bug class called, and how does your solution avoid it? Why does it
become relevant for your group's SWEB exactly when A1 is done?

**Q5.** (a) The raw Linux system call returns `-errno`. Why does a libc
wrapper test `(unsigned long)r > -4096UL` rather than `r < 0`?
(b) Why must `errno` be thread-local? What happens to a global `errno`
once `pthread_create` works?

**Q6.** In the inline assembly for `syscall`, what can go wrong if you
leave out (a) the `rcx`/`r11` clobbers, (b) the `"memory"` clobber,
(c) `volatile`? Give a concrete example for each.

**Q7.** Linux has `exit` (one thread) and `exit_group` (the whole
process). SWEB has one `sc_exit`, implemented as `currentThread->kill()`.
After A1, what must `sc_exit` do in a process with several threads, and
what does `pthread_exit` need instead? Why is it dangerous to kill another
thread *immediately*, wherever it currently is?

**Q8.** Follow `write(1, "hi", 2)` in a SWEB user program from
`userspace/libc/src/write.c` down to `kprintf` and back. Name every
function on the way, and mark where (a) the privilege level changes,
(b) the user registers are saved, (c) the return value is put into the
user's `rax`.

**Q9.** Your P1 task: `pthread_create(pthread_t *thread, const
pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)` as a
SWEB system call. For each argument: is it a pointer the kernel will
dereference? Which checks does it need, and *when* — before or after the
new thread is created? Which argument must the kernel *not* check?
