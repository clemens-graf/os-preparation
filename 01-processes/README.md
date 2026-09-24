# Module 01 — Processes: fork, exec, wait, pipes, signals

**Time:** 3–4 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 1)

A process is a running program with its own address space. Team F builds
`fork` and `exec` for SWEB, and in the interview *you* must be able to
explain their code — so this module matters for you too, even as P1.

## Steps

1. **Read** chapter 1 (process model, address-space layout, fork/exec/wait,
   zombies, file descriptors and pipes, signals).
2. **Examples** — `cd example && make run`, read in this order:
   - `fork_basics.c` — one call, two returns; separate copies of memory
   - `exec_wait.c` — the fork/exec/wait pattern and exit-status decoding
   - `zombie.c` — a zombie made visible in `/proc`
   - `stdio_trap.c` — why output appears twice after fork
   - `pipe_demo.c` — `ls / | wc -l` built by hand with `pipe` + `dup2`
   - `signals.c` — handlers and timer "interrupts" (run it yourself)
3. **Assignment** — implement the marked functions in `assignment/minish.c`,
   a mini shell (spec at the top of the file):
   ```bash
   cd assignment && make test      # automatic tests
   make && ./minish                # try it by hand
   ```
   The pipe (`a | b`) is a bonus; its test does not count as a failure.
4. **Paper questions** — `assignment/questions.md` (fork puzzles and a SWEB
   preview question about how `fork` can "return twice").

## Done when

- `make test` shows `core: 10 passed, 0 failed`.
- You can explain *why* the tests `04_output_order` and
  `05_no_duplicate_output` fail if you forget `fflush` or use `exit`
  instead of `_exit` in the child.

Reference solution and answers: `../solutions/01-processes/`.
