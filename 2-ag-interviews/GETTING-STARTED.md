# Getting started – how to train, step by step

How a training day works, and every command you type, with an example for each.
All commands work **from any directory**.

**Short version:** [TRAINING-PLAN.md](TRAINING-PLAN.md) says what to do today → for each task:
theory section in `concepts.pdf` → task card → triage → `task start` + timer → `task test` →
tick it in the plan. Every day: Anki 15 min + `task quiz 5`. Details: section 2.

## 0. What is where

| What | Where | You … |
|---|---|---|
| Course: task cards, examples, solutions | `~/Documents/University/2026-fall/os/preparation/2-ag-interviews/` | **read** here |
| What to do today | [`TRAINING-PLAN.md`](TRAINING-PLAN.md) (14-day plan, 19 Oct – 1 Nov) | tick tasks off |
| The theory behind each task (heap, sbrk, exit, registers, scheduler, locks …) | [`concepts.pdf`](concepts.pdf) – each task card names its section | read **before** a task |
| Where to change SWEB, recipes, locking | [`guide.pdf`](guide.pdf) | read once, look things up |
| Every file of your team's code | [`ATLAS.md`](ATLAS.md) | look things up |
| "What kind of task is this?" (U / S / E / A / K) | [`TRIAGE.md`](TRIAGE.md) | 30 s before every task |
| Worked examples, one per mechanism | [`examples/`](examples/README.md) (ex01 … ex14) | before the tasks that use them |
| Flashcards | [`anki/ag-interviews.apkg`](anki/README.md) | 15 min a day |
| Training copy of SWEB (a local git repo with your team's code) | `~/Documents/University/2026-fall/os/repos/sweb-ag/` | **write code** here |
| Build folder of the training copy | `/tmp/sweb-ag/` | never touch (logs are here) |
| The tool | `preparation/2-ag-interviews/tools/task.sh` | call it as `task` |

`repos/sweb-ag` is purely local, and `task` never pushes (a copy created by `task setup` has no
remote at all). The only fetch is `task team setup`, which copies one commit *from* your local
team repo into the training copy. Your practice repo `repos/sweb` and the team repo `osw26e3`
are never changed.

## 1. One-time setup

Create the shortcut `task`:

```bash
echo 'alias task=~/Documents/University/2026-fall/os/preparation/2-ag-interviews/tools/task.sh' >> ~/.bashrc
```

Open a **new terminal** (the alias only exists in new terminals), then check:

```bash
task list
```

Expected: first one line `importing your team's code …` (only the very first time), then a
table with every example and task (`ex01 … ex14`, `a0-01 … a0-14`, `a1-01 … a1-20`,
`a2-01 … a2-13`). The training repo exists already. Only on another laptop, or after deleting
it, do you need (your team repo must be cloned to `repos/osw26e3` first):

```bash
task setup
```

## 2. The daily loop

About 1–1.5 h a day. Which tasks: today's row in [TRAINING-PLAN.md](TRAINING-PLAN.md).

1. **Redo (≤ 20 min):** every task from **2 days ago** that you only finished with hints or the
   solution: `task reset <id>`, then from scratch, without looking.
2. **For each of today's tasks:**
   1. **Theory:** read the section of `concepts.pdf` the card names (line *Theory first*) and
      answer its *Check yourself* questions out loud. If the card names an example, look at it
      (`task example exNN`, then `task start <id>` for the task) – it shows the *how* on a
      different case.
   2. **Card:** read the spec block of `a1/NN-…/README.md` – **not** the section *Your team's
      code* yet: that is your one hint.
   3. **Triage (30 s, out loud):** the type (U / S / E / A / K) and the first file.
   4. **Implement in the time box** (the card says how long):
      ```bash
      task start a1-09                  # branch task/a1-09-vtop-ptov, test programs copied in
      task test                         # build + run the tests headless
      ```
      Done when only `[PASS]`, `0 failed`, `sweb_run status 0`.
   5. **Stuck when the time is up:** read *Your team's code* (the hint), 10 more minutes. Then
      `task diff <id>` + the solution's `NOTES.md`, every line out loud with its *why*.
   6. **Commit** your attempt and **tick it** in the plan – or mark it *needs redo*.
3. **Anki 15 min** (due cards) + `task quiz 5`.

```
TRAINING-PLAN → concepts.pdf → card (spec only) → triage → task start + timer → task test
                                                                     │
                       stuck? hint → 10 min → task diff + NOTES.md ←─┘     → commit, tick, next
```

```bash
task start a1-09
# ... implement in repos/sweb-ag (CLion: File → Open → repos/sweb-ag) ...
task test
cd ~/Documents/University/2026-fall/os/repos/sweb-ag && git add -A && git commit -m "a1-09: vtop/ptov works"
task start a0-14                        # next task - no reset needed, every task has its own branch
```

`task` refuses to switch to another task while you have uncommitted changes. Commit them
(the `git commit` line above) or throw them away with `task reset` (a backup is kept).
`task reset` is for **redoing** a task later, not for moving on.

## 3. Every command with examples

### `task list` – overview

```bash
task list
```
```
id     folder                         base    state
ex01   examples/ex01-syscalls         main    started
a0-01  a0/01-meminfo                  main    started, solution branch  <- current
a0-09  a0/09-pthread-multi            a0-08
```
*started* = you have a `task/…` (or `example/…`) branch, *solution branch* = you looked at the
solution, `<- current` = the branch `repos/sweb-ag` is on now. *base* = what the task starts
from: `main` (your team's code) or the solution of another task (e.g. A1-16/A1-17 build on
A1-15, A2-02 … A2-06 on A2-01 – `task start` sets that up for you).

### `task status` – where am I?

```bash
task status
```
Shows the branch, the task it belongs to, uncommitted changes and the number of backups.

### `task example exNN` – apply a worked example

```bash
task example ex01                       # branch example/ex01-syscalls, example applied
git -C ~/Documents/University/2026-fall/os/repos/sweb-ag show --stat    # which files it changed
git -C ~/Documents/University/2026-fall/os/repos/sweb-ag show           # the full change
task test                               # its test
task run                                # boot it, play with it
task reset ex01                         # undo your experiments: back to the unchanged example
```
Walkthrough of each example: `examples/exNN-…/README.md`; which tasks share which mechanism:
[examples/README.md](examples/README.md). ex11 and ex12 are the two A0 tutorials exactly as the
tutors wrote them (with their bugs – that is the lesson). ex13 = timer + scheduler, ex14 = a
function in the libc only (the `atexit` kind).

### `task start <id>` – begin (or continue) a task

```bash
task start a1-09                        # new: branch task/a1-09-vtop-ptov from main (your team's code) + tests
task start a1-16                        # builds on A1-15: starts from main + A1-15's solution
task start a1-09                        # again later: just switches back to your attempt
```
The ids accept several spellings: `a0-01`, `a0/01`, `a0-01-meminfo`.

### `task test [id]` – build and run the tests

```bash
task test                               # the task of the current branch
task test a0-01                         # name it explicitly (e.g. when on main)
```
Builds `repos/sweb-ag` (cmake runs every time, so new test files are found), boots SWEB
without a window, types the task's test programs into the shell, prints the results. Takes
20–60 s. **Done when:** only `[PASS]` lines, `0 failed`, `sweb_run status 0`. How to read the
output: section 5.

### `task run` / `task kvm` – boot SWEB in a window

```bash
task run                                # QEMU window (like make qemu)
task kvm                                # the same with hardware virtualisation - much faster
```
SWEB boots right away (your team's GRUB menu has `timeout 0`); then type a program, e.g.
`vtop_basic.sweb`, `help` or `ls`. Stop with **Ctrl+C in the terminal** (or close the window).
The kernel's debug output appears in the terminal.

### `task debug` + `task gdb gdb` – the debugger

Terminal 1:
```bash
task debug                              # debug build, QEMU starts and WAITS for the debugger
```
Terminal 2:
```bash
task gdb gdb                            # attach gdb (cgdb is not installed, so name gdb)
```
In gdb:
```
(gdb) break Syscall::write              # stop there
(gdb) continue                          # let SWEB run
(gdb) bt                                # backtrace when it stops
(gdb) print *currentThread              # look at variables
(gdb) next / step / finish              # step over / into / out
```
Back to the normal (faster) build afterwards:
```bash
task nodebug
```
(`task gdb` alone uses cgdb – install it with `sudo apt install cgdb` if you like it; `task gdb
ddd` for ddd.)

### `task solution <id>` – look at the reference solution

```bash
task solution a0-01                     # branch solution/a0-01-meminfo, switched to it
task test                               # it passes - run and read it
xdg-open ~/Documents/University/2026-fall/os/preparation/2-ag-interviews/solutions/a0/01-meminfo/NOTES.md
task start a0-01                        # back to your own attempt
```
`NOTES.md` explains the solution step by step: files, locking, pitfalls, answers to the card's
questions. The patch itself: `solutions/a0/01-meminfo/solution.patch`.

### `task diff <id>` – your attempt vs. the solution

```bash
task diff a0-01                         # kernel + libc, without the test programs
task diff a0-01 | less                  # page through it
```
Compares your **committed** attempt (branch `task/a0-01-meminfo`) with the solution.
Graphically in CLion: branch menu (bottom right) → `solution/a0-01-meminfo` → *Compare with Current*.

### `task reset …` – start over

```bash
task reset a0-01                        # your attempt → backup branch, fresh task/a0-01-… , switched to it
task reset ex02                         # an example back to its original state
task reset a0                           # every started A0 task (also a1, a2, ex, all)
```
Nothing is lost: the old attempt (including uncommitted changes) is kept as
`backup/a0-01-meminfo-<date>-<time>`.

### Your team's code

Every task and example starts from **your team's code**: the commit in `team/BASE` of your
team repo `osw26e3`, imported into the training copy as branch `main` (`task` does that by
itself the first time; it only *reads* `repos/osw26e3`). That is the code you have in the
interview: `currentThread->getProcess()`, `getAddressSpace()`, `getThreadLock()`, `UserThread`.

```bash
task team setup                         # import the team code again (e.g. after team/BASE changed)
```
Your team's names and every
task's place in your code: [`team/README.md`](team/README.md); every file: [`ATLAS.md`](ATLAS.md).

### `task quiz [N]` – "where is …?" questions

```bash
task quiz 5                             # 5 random questions (task quiz 20 on the redo day)
```
A question appears ("Where is the big switch over all syscall numbers?") – answer it out loud,
look it up with `git grep` / CLion if you are unsure, press **Enter**: the answer and the file
appear. 44 questions, from `tools/quiz.txt`; nothing in your repo is touched.

## 4. Git in the training repo

The tool works with branches; your own work you commit by hand. First go into the repo:

```bash
cd ~/Documents/University/2026-fall/os/repos/sweb-ag
```
```bash
git status                              # what did I change?
git diff                                # the changes themselves
git add -A && git commit -m "a0-01: first version"   # save the attempt (do this often)
git log --oneline -5                    # your commits on this task
git restore common/source/kernel/Syscall.cpp         # throw away changes in ONE file
```
Old attempts (from `task reset`):
```bash
git branch --list 'backup/*'                                    # all backups
git diff task/a0-01-meminfo backup/a0-01-meminfo-20261007-153000  # compare with the current attempt
git restore --source backup/a0-01-meminfo-20261007-153000 -- common/source/kernel/Syscall.cpp   # take one file back
```
Commit messages are free here – but practising the team style does not hurt:
`git commit -m "feat: add meminfo syscall"`.

## 5. Reading the test output

A good run:
```
== build /home/clem/.../repos/sweb-ag -> /tmp/sweb-ag
== run: note_basic.sweb note_errors.sweb exit
===== run 1: note_basic.sweb ===================
[PASS] a new thread has an empty note
[NOTE       ]/usr/note_basic.sweb: note is now "hello"     ← kernel debug line (your flag)
[PASS] setnote("hello"): 5
...
===== run 1: exit ==============================
== 23 passed, 0 failed, sweb_run status 0 (0 ok, 1 timeout/hang, 2 panic, ...)
== full kernel log: /tmp/sweb-ag/run.log
```

| You see | Meaning | Next step |
|---|---|---|
| `[FAIL] …` | a check failed – the text says which | read the test program in `userspace/tests/` |
| `sweb_run status 1` | **timeout**: a program did not come back within 30 s | deadlock, lost wake-up, endless loop |
| `status 2` + `KERNEL PANIC: Assertion …` | the kernel crashed – file and line are in the message | read that line; often a double free, a lock held twice |
| `status 3` | SWEB did not even reach the shell | crash during boot, or the shell is missing from the disk image (build again) |
| `status 4` + `leaking` | `exit` at the end found physical pages that were never freed | something mapped/allocated without freeing |
| `status 5` | a run that should panic (ex12) did not | – |
| `== expected 23 [PASS] lines, got 15` | a test program died half-way (it prints no `[FAIL]` then) | look for `EXIT: called, exit_code: 666/888/9999` above |
| `EXIT: called, exit_code: 666` | the process touched an address no segment covers | a wrong pointer – in the test or in your kernel code |
| `== BUILD FAILED` | compile error, shown above | full log: `/tmp/sweb-ag/make.log` |

The whole kernel log of the last run is `/tmp/sweb-ag/run.log`:
```bash
less /tmp/sweb-ag/run.log
```

## 6. Running tests by hand

`task test` calls `tools/run_test.sh`. You can call it yourself with any program list:

```bash
T=~/Documents/University/2026-fall/os/preparation/2-ag-interviews/tools
$T/run_test.sh meminfo_basic.sweb                     # one program
$T/run_test.sh meminfo_basic.sweb mult.sweb exit      # several, 'exit' = leak check at the end
$T/run_test.sh -n 5 pthread_create_basic.sweb         # 5 boots in a row: races show up
$T/run_test.sh --raw upcall_basic.sweb                # the whole kernel log, not just the summary
$T/run_test.sh --expect-pass 23 alias_basic.sweb      # also fail if fewer [PASS] lines appear
$T/run_test.sh "&fkey_swap_check.sweb" "!sleep 1" "!key f3" help   # start, wait, press F3
```
Special commands: `&prog` = start without waiting, `!sleep N` = wait, `!key f3` = press a key.

## 7. Problems and fixes

| Problem | Fix |
|---|---|
| `task: command not found` | open a new terminal (alias), or call `~/…/2-ag-interviews/tools/task.sh` |
| `you have uncommitted changes on … - commit them first` | `git add -A && git commit -m wip` in `repos/sweb-ag`, or `task reset <id>` |
| `which task? … not a task branch` | `task test a0-01`, or `task start a0-01` first |
| `unknown task 'a0-1'` | `task list` shows the ids (`a0-01`, not `a0-1`) |
| build error about `size_t` in `nonstd.h` | add `#include "types.h"` there |
| build error `unused parameter` / `reorder` | kernel warnings are errors: use the parameter or `(void) name;`; initializer list in declaration order |
| `Unknown command: x.sweb` in SWEB | when building yourself (CLion, `make`): reload CMake and build **twice**; `task test` does both |
| `disk image full of old programs - recreating it` | nothing to do, handled automatically |
| a libc task breaks the build of an unrelated test program | a name clash: tests are linked with the whole libc (ex14: `mult.c` has a global `rand`) |
| `task gdb`: cgdb not found | `task gdb gdb` |
| a test hangs for 30 s | it is a timeout (status 1): check `while` loops, sleeping without a wake-up, locks |

## 8. The plan until the interviews

| When | What |
|---|---|
| 19 Oct – 1 Nov | [TRAINING-PLAN.md](TRAINING-PLAN.md): 23 tasks in 14 days (all A1 tasks + the A0 tasks that come back in A1), the loop of section 2 |
| ~20–24 Oct | Student Debate A1 – your crib sheet |
| ≈ 30 Oct | Design-PoC deadline – the plan has only one task that day |
| 2 – 15 Nov | the rest of the plan's sections 1–6 (A2 tasks, `task quiz 20`, redo everything marked *needs redo*) |
| the week before 16–20 Nov | TRAINING-PLAN section 7: tasks you have not touched for a week, one of every type, `git pull` in the team repo + `task team setup` if `team/BASE` changed |

**Simulate the interview** at least twice: a task you have not seen for a few days →
`task reset <id>`, timer on, explain out loud while you write, then answer the card's
*questions a tutor might ask*.

## 9. Cheat sheet

```bash
task list                    # all tasks and their state
task status                  # where am I?
task example ex13            # apply an example
task start a1-09             # begin / continue a task
task test                    # build + run the tests
task run                     # boot in a window (task kvm: faster)
task debug                   # debug boot, waits for gdb ...
task gdb gdb                 # ... attach from a 2nd terminal; task nodebug afterwards
task solution a1-09          # the reference solution
task diff a1-09              # my attempt vs. the solution
task reset a1-09             # start over for a redo (backup kept)
task quiz 5                  # 5 "where is …?" questions (task quiz 20: more)
cd ~/Documents/University/2026-fall/os/repos/sweb-ag && git add -A && git commit -m "wip"   # save
```
