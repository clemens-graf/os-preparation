# Module 03 — Race conditions

**Time:** 3 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 3)

The moment two threads share data, correctness depends on timing. This
module teaches you to *see* races: in the machine code, in the set of
possible interleavings, and in ThreadSanitizer reports.

## Steps

1. **Read** chapter 3 (lost update, interleavings, check-then-act, critical
   sections, atomicity, data race vs. race condition, happens-before).
2. **Examples** — `cd example && make run`, then:
   - `make asm` — the three instructions behind `counter++`
   - `make tsan` — how ThreadSanitizer reports a race
   - read `interleave.c`: it enumerates *all* 20 interleavings of two
     `x = x + 1` threads (only 2 are correct) and all schedules of a racy
     bank withdrawal
   - `check_then_act.c` (races without `++`) and `three_fixes.c` (mutex vs.
     atomic vs. "don't share" — with timings)
3. **Assignment** — `bank.c`, `stats.c`, `once.c` all *work* single-threaded.
   Find every race, write a comment naming it, and fix it:
   ```bash
   cd assignment && make test          # all three parts
   make test-bank                      # or one part at a time
   ```
   Each part runs twice: under ThreadSanitizer, and optimised at full speed
   (where races show up as broken invariants).
4. **Paper questions** — `assignment/questions.md`.

## Done when

- `make test` prints `==== all parts pass`.
- You can explain why a single-core machine (like SWEB in QEMU) has the
  same races as a multicore one.

Reference solution and answers: `../solutions/03-race-conditions/`.
