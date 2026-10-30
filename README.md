# OS preparation

Two courses in one git repository (github.com/clemens-graf/os-preparation).

## [`1-foundations/`](1-foundations/)

The self-study course from September – background for everything:

- modules 00–11: C toolbox, processes, threads, races, locks, condition variables,
  deadlocks, syscalls, virtual memory, scheduling, a user-level thread library,
  the SWEB bridge
- the handbook (`theory/handbook.pdf`)

## [`2-ag-interviews/`](2-ag-interviews/)

Training for the live-coding part of the Abgabegespräche (A1: 16–20 Nov 2026):

- tasks on your team's code: A0 (13), A1 (23), A2 (13), each with tested solutions
- 13 worked examples (`examples/`)
- `concepts.pdf` (the theory, with a task → section map at the front)
- `guide.pdf` (what happens where in SWEB, recipes, locking)
- `TRIAGE.md`, `ATLAS.md`, `task.sh` (+ quiz), an Anki deck

It works in its own copy of SWEB, `../repos/sweb-ag`. Start with
[GETTING-STARTED.md](2-ag-interviews/GETTING-STARTED.md) (every command, with examples),
then its [README](2-ag-interviews/README.md).
