# A0-13 atexit – solution notes

**Your team's `main` has this now (your own `feature/atexit`, 18 Oct)** – the solution patch is
empty, `task test` checks your team's code. The code below is the same idea; your team's version
keeps `atexit_count` as `size_t` and calls the list from `__runAtexitFunctions()` in `exit()`.

Patch: [`solution.patch`](solution.patch) – only libc, nothing in the kernel. Type U (see
[TRIAGE.md](../../../TRIAGE.md)).

```c
// stdlib.c
#define ATEXIT_MAX 32
static void (*atexit_functions[ATEXIT_MAX])(void);
static int atexit_count = 0;

int atexit(void (*function)(void))
{
  if (function == 0 || atexit_count == ATEXIT_MAX)
    return -1;
  atexit_functions[atexit_count] = function;
  atexit_count++;
  return 0;
}

void __runAtexitFunctions(void)          // called by exit(), not by _exit()
{
  while (atexit_count > 0)
  {
    atexit_count--;                                          // take it off first ...
    void (*function)(void) = atexit_functions[atexit_count];
    function();                                              // ... a nested exit() continues
  }                                                          // with the remaining ones
}

// exec.c
extern void __runAtexitFunctions(void);
void exit(int status) { __runAtexitFunctions(); __syscall(sc_exit, status, ...); }
// _exit: unchanged - no handlers
```

- **Where:** `static` variables in `stdlib.c` – every process has its own copy (own address space).
- **Reverse order:** later registrations may depend on earlier ones (cleanup like a stack).
- **Pop before calling:** a handler that calls `exit()` again continues with the rest instead
  of running itself forever (`atexit_exit.c` tests that, with `_exit` inside the handler).
- `_start` (`nonstd.c`) calls `exit(main())` – returning from `main` runs the handlers too.
- **Threads:** two threads registering at the same moment can lose one function (both read the
  same `atexit_count`). For the interview: say it, and the fix – a lock (A1-15) or
  `__sync_fetch_and_add(&atexit_count, 1)` to reserve the slot.
- Compiler trap from the test: `exit` is not declared `noreturn` in SWEB's libc, so GCC reports
  "infinite recursion" for a function that ends every path with `exit()` – restructure.
