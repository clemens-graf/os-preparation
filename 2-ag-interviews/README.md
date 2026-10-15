# AG interview training (A0, A1, A2)

Practice for the **live coding part of the Abgabegespräche**: the tutor hands you a small
task, you implement it in SWEB while they watch, and you explain what you did and why.
This course gives you tasks of the same shape as the ones students reported, worked
reference examples, tested solutions, and a theory guide that answers the question you
will ask yourself every time: **which file do I have to touch for this?**

| | |
|---|---|
| [`GETTING-STARTED.md`](GETTING-STARTED.md) | **start here:** every command you type, with examples – setup, the daily loop, tests, debugging, git, reading the output, problems |
| [`TRAINING-PLAN.md`](TRAINING-PLAN.md) | **the checklist:** every task, grouped by type – work through it and you have practised every kind of task (not only syscalls) |
| [`anki/`](anki/) | an **Anki deck** (269 cards): task types, every file of your team's code, the API, locking, traps, POSIX – double-click `anki/ag-interviews.apkg` |
| [`TRIAGE.md`](TRIAGE.md) | **an unknown task in 5 minutes:** the five task types (user space only, syscall, kernel event, assembly, kernel calls user code), where each one goes, every course task classified |
| [`ATLAS.md`](ATLAS.md) | **your team's code, file by file:** what is in every file, the call chains (program start/end, syscall, page fault, timer, key, pthread_create, fork), "I want to … → go to", the POSIX stubs of your libc |
| [`guide.pdf`](guide.pdf) | The theory: what happens where in SWEB, recipes for every kind of change, **locking** (the biggest source of deductions), the A0/A1/A2 task families, your team's code |
| [`examples/`](examples/) | 11 worked reference changes on **your team's code** with walkthroughs: syscalls, mapping pages, page faults, PTE flags, registers, kernel threads, F-keys, disk, loader, and the two A0 tutorials (ex11, ex12) as the tutors wrote them |
| [`a0/`](a0/) | 13 tasks for the A0 interview, incl. both tutorials done right (A0-11, A0-12), `atexit` (A0-13, the real A0 task of 2026), string functions (A0-14), and the [cheat sheet](a0/cheatsheet.pdf) |
| [`a1/`](a1/) | 20 tasks for the A1 interview – threads, scheduling, processes, and every POSIX stub of your libc (malloc, locks, semaphores, condition variables, sleep, waitpid, detach) |
| [`a2/`](a2/) | 13 tasks for the A2 interview – swapping, page-table bits, kernel threads, F-keys, mmap |
| [`solutions/`](solutions/) | a tested solution for every task on **your team's code**: `solution.patch` + `NOTES.md` (files, steps, locking, pitfalls, answers) |
| [`team/`](team/) | which commit of your team's code the course runs on (`BASE`), your team's names, every task's place in your code, bugs found – [`team/README.md`](team/README.md) |
| [`tools/`](tools/) | `task.sh` (start / test / reset / solution / run / debug / **quiz**), `run_test.sh`, `sweb_run.py`, `tasks.txt`, `quiz.txt` |
| [`theory/`](theory/) | LaTeX sources of the guide (`cd theory && make`) |

Every task and example was implemented and run on your team's code: all of them pass their tests
with the reference solution (checked end to end through `task.sh`) – except the study tasks A1-01
and A1-06, whose tests show two bugs in your team's `pthread_join`/`pthread_cancel` (see their notes).

## Where you work: the training copy `repos/sweb-ag`

A separate repository at `~/Documents/University/2026-fall/os/repos/sweb-ag`, filled with your
team's code (read from `repos/osw26e3`, never changed there; no remote – nothing can be pushed
from it). It is a normal SWEB checkout: CMake, `make qemu`, `make kvm`,
`make debug` + gdb, CLion – everything works exactly as in your other SWEB repos. Build folder:
`/tmp/sweb-ag`.

The tasks run on **your team's code**: `task.sh` imports your team's `main` (the commit in
[`team/BASE`](team/BASE), read from `repos/osw26e3`) as branch `main`, and every A0 task and
example starts from there – the code you have in the interview. Every task lives on its **own
branch**, created by `task.sh`:

| Branch | What |
|---|---|
| `main` | your team's code (pinned commit) |
| `task/a0-01-meminfo` | your work on a task (created with the test programs already in `userspace/tests/`) |
| `solution/a0-01-meminfo` | the reference solution, to build, run and compare |
| `example/ex02-map-page` | an example applied |
| `base/a0-08` | for tasks that build on another task's solution (A0-09, A1-15 …) – created automatically |
| `backup/a0-01-meminfo-<date>` | your attempt, kept whenever you reset |

## Quick start

Put the tool on your path once (or call it with its full path):

```bash
echo 'alias task=~/Documents/University/2026-fall/os/preparation/2-ag-interviews/tools/task.sh' >> ~/.bashrc
```

Then, in a new terminal (`setup` only does something if `repos/sweb-ag` does not exist yet –
e.g. on another laptop):

```bash
task setup
```
```bash
task list
```
```bash
task start a0-01
```

That puts you on branch `task/a0-01-meminfo` in `repos/sweb-ag`, with `meminfo_basic.c` in
`userspace/tests/`. Open the task card `a0/01-meminfo/README.md`, implement it in
`repos/sweb-ag` (any editor, CLion: open `repos/sweb-ag`), then:

```bash
task test
```

builds it and runs the task's test programs without a window (with SWEB's leak check at the
end). Done when it reports only `[PASS]` lines, `0 failed` and status 0.

| Command | Does |
|---|---|
| `task start a0-02` | next task (commit or reset your current one first – the tool refuses to switch with uncommitted changes) |
| `task test` | build + run the tests of the task you are on |
| `task run` / `task kvm` | build + boot SWEB in a window, like `sweb-practise` – type the test program's name in the shell |
| `task debug`, then in a 2nd terminal `task gdb` | debug build, QEMU waits for the debugger (`make qemugdb` / `make runcgdb`); `task nodebug` goes back |
| `task reset a0-01` | **start the task over from scratch** – your attempt is kept in a `backup/...` branch |
| `task reset a0` | the same for every task of A0 you started (`a1`, `a2`, `ex`, `all`) |
| `task solution a0-01` | branch with the reference solution, switched to it (build/run it with `task test` / `task run`) |
| `task diff a0-01` | your attempt vs. the solution (kernel + libc) |
| `task example ex02` | an example applied on its own branch |
| `task status` | where you are, uncommitted changes, number of backups |
| `task quiz` / `task quiz 20` | random "where is …?" questions about your team's code, answer after Enter ([`ATLAS.md`](ATLAS.md) has them all) |

A typical round: `task start a0-03` → try → stuck? hints on the card → still stuck?
`task solution a0-03`, read `solutions/a0/03-heaplimit/NOTES.md` → `task reset a0-03` → write it
again from scratch without looking.

## How to train for the interview

What counts is **finding the right place fast, writing a minimal correct version, and
explaining it – especially the locking.**

0. **Every task, first five minutes: classify it** ([`TRIAGE.md`](TRIAGE.md)) – user space only,
   syscall, kernel event, assembly, or kernel calls user code? Then find the starting point
   ([`ATLAS.md`](ATLAS.md)) and say both out loud. `task quiz` trains the finding.
1. **First pass – learn.** Guide chapters 1–4. Then the examples: `task example ex01`, read its
   `README.md` and the diff (`git -C ~/Documents/University/2026-fall/os/repos/sweb-ag show`),
   `task test`. Ask yourself for every line *why here?*
2. **Second pass – solve.** The tasks in order, time-boxed (the card says how long). Stuck: one
   hint at a time. Then compare with `NOTES.md` – especially *Locking* and *Pitfalls*.
3. **Third pass – simulate.** A task you have not looked at for a few days: `task reset`, start a
   timer, **talk out loud** while you work. Then answer the card's *questions a tutor might ask*.

The tutor stops you as soon as it is clear you deserve the points. A short correct version with
the right lock and a clear explanation beats a long one that you cannot explain.

## A0 (interview 13–17 Oct)

What students reported (`a0/README.md`): syscalls, page faults, mapping a page for user space,
register manipulation, "call a user function on segfault", the loader, the mechanics of
`pthread_create`, `pthread_multi` – plus what the tutors themselves practised in the two A0
tutorials: a user string, a debug flag and per-thread data (tutorial 1: ex11, A0-11), the
physical page of a variable and a second mapping of it (tutorial 2: ex12, A0-12). The A0
tasks cover exactly these. **But in the A0 of 14 Oct 2026 the task was `atexit` – pure libc, no
syscall at all** – it is [A0-13](a0/13-atexit/) now; A0-14 and A1-14 … A1-17 are more of that kind: classify every task first.
**Short on time:** guide chapters 2–4, examples ex01, ex11, ex02, ex12, ex03, ex05, then tasks
13, 01, 11, 02, 12, 03, 06, 08.

## Interview vs. training repo

In the interview you work in **your team repo** – and so does this course: every card, hint, note
and solution uses your team's names (`getProcess()`, `getAddressSpace()`, `getThreadLock()`,
`UserThread`, state in `UserProcess`). The tutorials (ex11, ex12) were written for upstream SWEB;
[`team/README.md`](team/README.md) shows how to read them. Spend 15 minutes in the team repo
itself before the interview – [`ATLAS.md`](ATLAS.md) is the map, `task quiz` trains finding things.

## Without task.sh

Everything also works by hand: `git switch -c my-try main`, copy the task's `.c` files to
`userspace/tests/`, build as usual; solutions are plain patches
(`git apply ~/.../solutions/a0/01-meminfo/solution.patch` on a clean `main`). `tools/run_test.sh
meminfo_basic.sweb` builds `$SWEB_SRC` (default `repos/sweb-ag`) and runs any test command list.
