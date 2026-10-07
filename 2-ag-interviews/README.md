# AG interview training (A0, A1, A2)

Practice for the **live coding part of the Abgabegespräche**: the tutor hands you a small
task, you implement it in SWEB while they watch, and you explain what you did and why.
This course gives you tasks of the same shape as the ones students reported, worked
reference examples, tested solutions, and a theory guide that answers the question you
will ask yourself every time: **which file do I have to touch for this?**

| | |
|---|---|
| [`GETTING-STARTED.md`](GETTING-STARTED.md) | **start here:** every command you type, with examples – setup, the daily loop, tests, debugging, git, reading the output, problems |
| [`guide.pdf`](guide.pdf) | The theory: what happens where in SWEB, recipes for every kind of change, **locking** (the biggest source of deductions), the A0/A1/A2 task families, base SWEB vs. your team repo |
| [`examples/`](examples/) | 11 worked reference changes with walkthroughs: syscalls, mapping pages, page faults, PTE flags, registers, kernel threads, F-keys, disk, loader, and the two A0 tutorials (ex11, ex12) exactly as the tutors wrote them |
| [`a0/`](a0/) | 11 tasks for the A0 interview (13–17 Oct), incl. both tutorials done right (A0-11, A0-12) |
| [`a1/`](a1/) | 14 tasks for the A1 interview |
| [`a2/`](a2/) | 12 tasks for the A2 interview |
| [`solutions/`](solutions/) | a tested solution for every task: `solution.patch` + `NOTES.md` (files, steps, locking, pitfalls, answers) |
| [`tools/`](tools/) | `task.sh` (start / test / reset / solution / run / debug), `run_test.sh`, `sweb_run.py`, `tasks.txt` |
| [`theory/`](theory/) | LaTeX sources of the guide (`cd theory && make`) |

Every task and example was implemented and run: all 48 pass their tests with the reference
solution (checked end to end through `task.sh`), and the tests fail without it.

## Where you work: the training copy `repos/sweb-ag`

A separate clone of upstream SWEB (`f6fcb2ab`, the same as your practice repo's `main`) at
`~/Documents/University/2026-fall/os/repos/sweb-ag`. Your practice repo `repos/sweb` and the
team repo are never touched. It is a normal SWEB checkout: CMake, `make qemu`, `make kvm`,
`make debug` + gdb, CLion – everything works exactly as in your other SWEB repos. Build folder:
`/tmp/sweb-ag`.

Every task lives on its **own branch**, created by `task.sh`:

| Branch | What |
|---|---|
| `main` | clean upstream SWEB |
| `task/a0-01-meminfo` | your work on a task (created with the test programs already in `userspace/tests/`) |
| `solution/a0-01-meminfo` | the reference solution, to build, run and compare |
| `example/ex02-map-page` | an example applied |
| `base/a0-08` | for tasks that build on another task's solution (A0-09, A1, A2) – created automatically |
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

A typical round: `task start a0-03` → try → stuck? hints on the card → still stuck?
`task solution a0-03`, read `solutions/a0/03-heaplimit/NOTES.md` → `task reset a0-03` → write it
again from scratch without looking.

## How to train for the interview

What counts is **finding the right place fast, writing a minimal correct version, and
explaining it – especially the locking.**

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
physical page of a variable and a second mapping of it (tutorial 2: ex12, A0-12). The eleven A0
tasks cover exactly these.
**Short on time:** guide chapters 2–4, examples ex01, ex11, ex02, ex12, ex03, ex05, then tasks
01, 11, 02, 12, 03, 06, 08.

## Interview vs. training repo

In the interview you work in **your team repo**, not in base SWEB. The mechanisms are the same; a
few names differ because the team split process and thread (`currentThread->loader_` is
`currentThread->getLoader()` there, per-process state belongs into `UserProcess`, not into the
`Loader`). The guide's last chapter has the translation table – read it, and spend 15 minutes in
the team repo before the interview.

## Without task.sh

Everything also works by hand: `git switch -c my-try main`, copy the task's `.c` files to
`userspace/tests/`, build as usual; solutions are plain patches
(`git apply ~/.../solutions/a0/01-meminfo/solution.patch` on a clean `main`). `tools/run_test.sh
meminfo_basic.sweb` builds `$SWEB_SRC` (default `repos/sweb-ag`) and runs any test command list.
