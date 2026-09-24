# Module 11 — SWEB bridge

**Time:** 6–7 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 11)

Modules 00–10 built every idea on your own laptop. This module walks the
same ideas through SWEB, and ends in the two things you need first in the
course: the **Phase 0 design** (split `UserProcess` from `Thread`,
1–4 Oct, all four of you) and a first **P1 design** for
`pthread_create`/`pthread_join`.

Nothing here changes your group repository. The hands-on parts use a
throwaway clone.

## Steps

1. **Read** chapter 11 (a map of SWEB, the boot path, how every earlier
   module shows up in the code, what A1 changes, how to test).
2. **Tool** — `tools/sweb_run.py` boots a SWEB build headless, runs shell
   commands and reports hangs (exit 1) and kernel panics (exit 2):
   ```bash
   tools/sweb_run.py -b /tmp/sweb help mult.sweb
   tools/sweb_run.py -b /tmp/sweb -n 10 my_test.sweb    # ten boots in a row
   ```
3. **Worksheet** — `worksheet.md`:
   - Part 0: build, boot, debug output, adding programs, gdb
   - Part 1: where is what
   - Part 2: six traces — boot, syscall, scheduler, page fault, thread
     death, locks
   - Part 3: Phase 0 design (bring it to the group meeting)
   - Part 4: P1 design and a test plan
   - Part 5 (optional): add a `gettid` system call in the playground
4. Compare with `../solutions/11-sweb-bridge/answers.md`.

## Done when

- You can boot SWEB from the command line and run a test program
  headless.
- You have a one-page Phase 0 sketch and a P1 test list.

Reference answers: `../solutions/11-sweb-bridge/`.
