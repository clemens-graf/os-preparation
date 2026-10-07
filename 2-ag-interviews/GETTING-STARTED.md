# Getting started – the task tool, step by step

Everything you type to train for the AG interviews, with an example for every command.
All commands work **from any directory**.

## 0. What is where

| What | Where | You … |
|---|---|---|
| Course: task cards, examples, solutions, `guide.pdf` | `~/Documents/University/2026-fall/os/preparation/2-ag-interviews/` | **read** here |
| Training copy of SWEB (a local git repo, upstream `f6fcb2ab`) | `~/Documents/University/2026-fall/os/repos/sweb-ag/` | **write code** here |
| Build folder of the training copy | `/tmp/sweb-ag/` | never touch (logs are here) |
| The tool | `preparation/2-ag-interviews/tools/task.sh` | call it as `task` |

`repos/sweb-ag` is purely local. Its only remote is the official SWEB on GitHub (no push
rights), and `task` never pushes or fetches. Your practice repo `repos/sweb` and the team repo
`osw26e3` are never touched.

## 1. One-time setup

Create the shortcut `task`:

```bash
echo 'alias task=~/Documents/University/2026-fall/os/preparation/2-ag-interviews/tools/task.sh' >> ~/.bashrc
```

Open a **new terminal** (the alias only exists in new terminals), then check:

```bash
task list
```

Expected: a table with every example and task (`ex01 … ex12`, `a0-01 … a0-12`, `a1-…`,
`a2-…`), all without a state. The training repo exists already. Only on another laptop, or
after deleting it, do you need:

```bash
task setup
```

## 2. The daily loop

```
task start a0-01   →  read the card  →  code in repos/sweb-ag  →  task test
       ↑                                                              │
       └──── task reset a0-01 (start over) ←── stuck? hints → task solution / task diff
```

```bash
task start a0-01                        # branch task/a0-01-meminfo, test programs copied in
xdg-open ~/Documents/University/2026-fall/os/preparation/2-ag-interviews/a0/01-meminfo/README.md
# ... implement in repos/sweb-ag (CLion: File → Open → repos/sweb-ag) ...
task test                               # build + run the tests headless
cd ~/Documents/University/2026-fall/os/repos/sweb-ag && git add -A && git commit -m "a0-01: meminfo works"
task start a0-02                        # next task
```

`task` refuses to switch to another task while you have uncommitted changes. Commit them
(last line above) or throw them away with `task reset` (a backup is kept).

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
from: `main` (plain SWEB) or the solution of another task (A0-09 and A1 build on A0-08, A2-02…06
on A2-01 – `task start` sets that up for you).

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
Walkthrough of each example: `examples/exNN-…/README.md`. ex11 and ex12 are the two A0
tutorials exactly as the tutors wrote them (with their bugs – that is the lesson).

### `task start <id>` – begin (or continue) a task

```bash
task start a0-01                        # new: branch task/a0-01-meminfo from main + tests
task start a0-09                        # builds on A0-08: starts from main + A0-08's solution
task start a0-01                        # again later: just switches back to your attempt
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
In the window: press **Enter** at the GRUB menu (it has no timeout), then type a program, e.g.
`meminfo_basic.sweb`, `help` or `ls`. Stop with **Ctrl+C in the terminal** (or close the window).
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
(gdb) continue                          # let SWEB run - press Enter at GRUB in the QEMU window
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
| QEMU window sits at a menu | press Enter (GRUB has no timeout) |
| `task gdb`: cgdb not found | `task gdb gdb` |
| a test hangs for 30 s | it is a timeout (status 1): check `while` loops, sleeping without a wake-up, locks |

## 8. Training plan until the A0 interview (13–17 Oct)

1. **Learn:** `guide.pdf` chapters 2–4 (what is where, recipes, locking).
2. **Examples**, each with `task example exNN`, its README and `task test`:
   ex01 → ex11 (tutorial 1) → ex02 → ex12 (tutorial 2) → ex03 → ex05
3. **Tasks** with `task start`, time-boxed (the card says how long):
   01 → 11 → 02 → 12 → 03 → 06 → 08
4. **Simulate the interview:** a task you have not seen for a few days → `task reset <id>`,
   timer on, explain out loud while you write, then answer the card's *questions a tutor might ask*.

## 9. Cheat sheet

```bash
task list                    # all tasks and their state
task status                  # where am I?
task example ex01            # apply an example
task start a0-01             # begin / continue a task
task test                    # build + run the tests
task run                     # boot in a window (task kvm: faster)
task debug                   # debug boot, waits for gdb ...
task gdb gdb                 # ... attach from a 2nd terminal; task nodebug afterwards
task solution a0-01          # the reference solution
task diff a0-01              # my attempt vs. the solution
task reset a0-01             # start over (backup kept)
cd ~/Documents/University/2026-fall/os/repos/sweb-ag && git add -A && git commit -m "wip"   # save
```
