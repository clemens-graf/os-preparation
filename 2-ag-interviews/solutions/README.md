# Solutions

One folder per task:

- `solution.patch` – the complete change against your team's code (the commit in
  `../team/BASE`), **without** the test programs (those are in the task folders).
  Some tasks build on a base – then there is also an `on-top-of-<base>.patch` with only the
  task's own part. An **empty** `solution.patch` = a study task: your team's code already has it
  (A0-08, A1-01, A1-06) – the tests run against your code.
- `NOTES.md` – the walkthrough: files touched, the key code, **locking** (which lock
  protects what and why), pitfalls (several of them are bugs I made myself while writing
  the solutions – the tests caught them), and answers to the questions on the task card.

Every solution was built and run on your team's code: all checks of its test pass (the study
tasks A1-01 and A1-06 show two bugs in your team's join/cancel – see their notes).

## Using a solution

```bash
task solution a0-01       # branch solution/a0-01-meminfo: main + solution + tests, switched to it
task test                 # it passes - run it, read it, change it
task diff a0-01           # your attempt (task/a0-01-...) vs. the solution
task start a0-01          # back to your attempt
task reset a0-01          # ... or start over from scratch to write it again yourself
```

Reading a patch directly: `+` lines are added, `-` removed, the rest is context; the
`@@ -a,b +c,d @@` header says where. By hand: `git apply .../solution.patch` on a clean `main`.

**Use the solution after your own attempt** – the interview tests whether you can find the
place and write it yourself. The *NOTES* are worth reading even when your solution works:
they contain the locking reasoning the tutors ask about.
