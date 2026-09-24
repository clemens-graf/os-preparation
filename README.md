# OS preparation course

A self-study course for Operating Systems at TU Graz (winter 2026/27),
written for someone who skipped SLP. It covers threads, synchronisation,
system calls, virtual memory and scheduling, and ends in SWEB with the
A1 tasks: the **Phase 0** process/thread split, then **P1**:
`pthread_create` and `pthread_join`.

Every module has four parts:
- a **theory chapter** (`theory.pdf`; all chapters together in `theory/handbook.pdf`);
- **examples** to run and read;
- an **assignment** with automatic tests;
- **paper questions**.

Reference solutions and answers are in `solutions/`.

## Roadmap

| # | Module | Time | Makes up for | Most useful for |
|---|---|---|---|---|
| 00 | [C toolbox](00-c-toolbox/) | 2–3 h | SLP | reading SWEB, sanitizers |
| 01 | [Processes](01-processes/) | 3–4 h | SLP | Phase 0, Team F's fork/exec |
| 02 | [Threads](02-threads/) | 3–4 h | SLP | P1 (pthread API semantics) |
| 03 | [Race conditions](03-race-conditions/) | 3 h | SLP / OS | everything after Phase 0 |
| 04 | [Locks](04-locks/) | 3–4 h | OS | SWEB `SpinLock`/`Mutex`, P1 elective |
| 05 | [Condition variables & semaphores](05-condvars-semaphores/) | 4–5 h | SLP / OS | `pthread_join`, P1 elective |
| 06 | [Deadlocks](06-deadlocks/) | 3 h | OS | lock order across both teams |
| 07 | [System calls & user pointers](07-syscalls/) | 3–4 h | OS | the `pthread_*` system calls |
| 08 | [Virtual memory](08-virtual-memory/) | 4–5 h | OS | thread stacks; A2 (CoW, swapping) |
| 09 | [Scheduling](09-scheduling/) | 3 h | OS | exam; join must sleep, not spin |
| 10 | [Capstone: your own thread library](10-uthreads/) | 5–6 h | OS | **P1 dress rehearsal** |
| 11 | [SWEB bridge](11-sweb-bridge/) | 6–7 h | OS | **Phase 0 and P1 design** |

About 45 hours in total. If time runs short before Phase 0 (1 Oct), do
02 → 03 → 05 → 07 → 10 → 11 first, then fill in the rest before the
exam.

## Working on a module

```bash
cd 07-syscalls
xdg-open theory.pdf                  # read the chapter
cd example && make run               # run and read the examples
cd ../assignment && make test        # implement, test, repeat
make test IMPL=../../solutions/07-syscalls    # how the reference does
```

The tests build your code twice. One build runs under a sanitizer:
AddressSanitizer, ThreadSanitizer or UBSan, depending on the module. The
other is optimised (`-O2`), where timing bugs become visible. A hanging
test is stopped after a timeout with a hint (deadlock? lost wake-up?).

## Requirements

`gcc`, `make`, `python3`; for module 11 also `qemu-system-x86_64` and a
`cmake` (CLion's bundled one works); for rebuilding the handbook,
`pdflatex` (`cd theory && make`).

## Layout

```
NN-topic/README.md          steps, "done when"
NN-topic/theory.pdf         the chapter (built from theory/chNN.tex)
NN-topic/example/           programs to run and read (make run)
NN-topic/assignment/        skeleton + tests (make test) + questions.md
solutions/NN-topic/         reference solution + answers.md
common/                     check.h (test macros), common.mk, run.sh
theory/                     LaTeX sources, handbook.pdf
11-sweb-bridge/tools/       sweb_run.py: run SWEB headless, detect hangs/panics
```
