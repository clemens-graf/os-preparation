# Anki deck – AG interviews

**[`ag-interviews.apkg`](ag-interviews.apkg)** – 269 cards about **your team's code** (the commit
in `../team/BASE`), deck `OS AG` with 9 sub-decks:

| Sub-deck | Cards about |
|---|---|
| 1 Triage | the five task types, "task X – which type, which file first?" |
| 2 Where is what | the call chains and places from [ATLAS.md](../ATLAS.md) |
| 2b Every file | **every file** of your team's x86-64 code: "what is in …?" and "which file …?" – kernel, memory, interrupts, drivers, console, file systems, util, libc, tests, build |
| 3 Team API | `getProcess()`, `getAddressSpace()`, `getThreadLock()`, `UserThreadData`, … |
| 4 Locking | the rules the tutors ask about |
| 5 Traps | the bugs that cost time in the interview |
| 6 Tasks | the key idea of the important tasks |
| 7 POSIX | what the standard functions must do (atexit, join, waitpid, strncpy, …) |
| 8 Basics | pages, frames, TLB, ELF |

## Import

Double-click `ag-interviews.apkg` (or Anki → **File → Import**). It brings its own card type
(`OS AG (basic)`, with code formatting) and the tags (`ag::…`). Importing a newer version later
**updates** the cards – your review history stays (every card has a fixed ID from its front).

`ag-interviews.txt` is the same deck as a plain-text import file (File → Import works too).

## Change or add cards

Edit the lists in [`make_deck.py`](make_deck.py), then rebuild both files – `build_apkg.py` uses
the `anki` library that comes with Anki itself, nothing to install:

```bash
cd ~/Documents/University/2026-fall/os/preparation/2-ag-interviews/anki
python3 make_deck.py
A=/usr/local/share/anki; LD_LIBRARY_PATH=$A/python/lib PYTHONHOME=$A/python PYTHONPATH=$A/app_packages $A/python/bin/python3 build_apkg.py
```

A card whose *front* you change becomes a new card (delete the old one in the browser).
