# ex14 – a function in the libc only (no syscall)

`qsort` (stdlib.h) sorts an array of any element type with a compare function, and `strtok`
(string.h) splits a string into tokens. **Nothing in the kernel changes.** That is the
`atexit` kind of task (type U, [TRIAGE.md](../../TRIAGE.md)). Test: `ex_libc.sweb` (7 checks).

Theory: [concepts.pdf](../../concepts.pdf) §7 C strings, §5 (atexit – the same kind of task).

## The places

| # | File | What |
|---|---|---|
| 1 | `userspace/libc/include/stdlib.h`, `string.h` | the **declaration**, before the `#ifdef __cplusplus }` at the end. Without it a test program does not compile (`-Werror`: implicit declaration) |
| 2 | `userspace/libc/src/qsort.c`, `strtok.c` | the implementation. New `.c` files are found by a glob, so cmake has to run again (`task test` always does) |
| 3 | `userspace/tests/*.c` | a program that uses it |

If the stub already exists (as `atexit`, `malloc` and `sem_*` do in your libc), you only fill
in place 2.

## Three things a libc function has to deal with

- **`void*` + element size (qsort):** the libc does not know the type, so it computes addresses
  in bytes, `(char*) base + i * size`, and lets the caller's function compare. A function
  pointer parameter: `int (*compar)(const void*, const void*)` – called like a function,
  `compar(a, b)`.
- **State inside the libc (strtok):** `static char *next_token` remembers where to continue. Every
  process has its own copy, because it is in the process' `.data`. **All threads of a process
  share it**, so `strtok` is not thread-safe. `atexit`'s list, `malloc`'s heap and `rand`'s seed
  work the same way.
- **Name clashes:** the test programs are linked with `-whole-archive`, so *every* libc function
  ends up in *every* program. A first version of this example had `rand()`, and the build broke:
  `mult.c` has a global variable called `rand`. If a libc task breaks the build of an unrelated
  test program, look for a name clash.

## Tasks that use this

A0-13 atexit (state: the list of functions; hook in `exit()`), A0-14 string functions,
A1-14 malloc (state: the heap), A1-15 spinlock/mutex, A1-16 semaphores, A1-17 condition
variables (state: inside the user's struct, plus an atomic instruction).
