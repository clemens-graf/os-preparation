# Module 00 — C toolbox for systems programming

**Time:** 2–3 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 0)

Everything later in this course (and in SWEB) assumes you can read and write
pointer-heavy C without thinking twice. The one declaration this module is
aiming at:

```c
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
```

By the end you should be able to explain every token of it: why `thread` is
a pointer (out-parameter), what `void *(*)(void *)` is, and why `arg` is a
`void *`.

## Steps

1. **Read** chapter 0 of the handbook (toolchain, sanitizers, pointers,
   lifetimes, function pointers).
2. **Example** — `cd example && make run`, then read the sources in this order:
   - `pointers.c`: addresses, out-parameters, `void *`, function pointers
   - `callbacks.c`: the *function pointer + context pointer* pattern
   - `lifetime.c`: stack vs heap vs static; then run `make bug-stack`,
     `make bug-uaf`, `make bug-leak` and read the first lines of each
     AddressSanitizer report — you will see these reports again.
3. **Assignment** — implement `assignment/toolbox.c` (spec in `toolbox.h`):
   - Part A: a growable `void *` array (`vec_*`)
   - Part B: `task_create` / `task_run` / `task_join` — the *shape* of the
     pthread API without threads
   - Part C: `split_words` — you will reuse this idea for the shell in module 01

   ```bash
   cd assignment && make test
   ```

## Done when

- `make test` prints `12 passed, 0 failed` and no sanitizer report.
- You can answer the "check yourself" questions at the end of chapter 0.

Reference solution: `../solutions/00-c-toolbox/` (look only after trying).
