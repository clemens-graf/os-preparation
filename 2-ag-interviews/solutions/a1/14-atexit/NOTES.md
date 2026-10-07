# A1-14 atexit – solution notes

Patch: [`solution.patch`](solution.patch) – only libc.

```c
// stdlib.c
static void (*atexit_functions[ATEXIT_MAX])(void);
static size_t atexit_count = 0;

int atexit(void (*function)(void))
{
  if (!function) return -1;
  size_t slot = __atomic_fetch_add(&atexit_count, 1, __ATOMIC_SEQ_CST);   // threads!
  if (slot >= ATEXIT_MAX) { __atomic_fetch_sub(&atexit_count, 1, __ATOMIC_SEQ_CST); return -1; }
  atexit_functions[slot] = function;
  return 0;
}

void __runAtexitFunctions(void)
{
  while (atexit_count > 0) {
    void (*f)(void) = atexit_functions[--atexit_count];   // pop first
    if (f) f();
  }
}

// exec.c
void exit(int status) { __runAtexitFunctions(); __syscall(sc_exit, status, ...); }
// _exit: unchanged - no handlers
```

- Reverse order: later registrations may depend on earlier ones (cleanup like a stack).
- Pop before calling: a handler that calls `exit()` again continues with the rest instead of
  looping on itself.
- `_start` calls `exit(main())` – returning from main runs the handlers.
- Compiler trap from the test: `exit` is not declared `noreturn` in SWEB's libc, so GCC
  reports "infinite recursion" for a function that ends every path with `exit()` – restructure.
