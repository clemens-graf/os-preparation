# Anki deck – AG interviews

[`ag-interviews.txt`](ag-interviews.txt): 127 cards about **your team's code** (the commit in
`../team/BASE`), in 8 sub-decks of `OS AG`:

| Sub-deck | Cards about |
|---|---|
| 1 Triage | the five task types, "task X – which type, which file first?" |
| 2 Where is what | the files and call chains of [ATLAS.md](../ATLAS.md) |
| 3 Team API | `getProcess()`, `getAddressSpace()`, `getThreadLock()`, `UserThreadData`, … |
| 4 Locking | the rules the tutors ask about |
| 5 Traps | the bugs that cost time in the interview |
| 6 Tasks | the key idea of the important tasks |
| 7 POSIX | what the standard functions must do (atexit, join, waitpid, strncpy, …) |
| 8 Basics | pages, frames, TLB, ELF |

## Import (once, and again after changes)

Anki → **File → Import** → choose `ag-interviews.txt` → *Import*. The file sets the note type
(Basic), the decks and the tags (`ag::…`) itself. Importing again updates the existing cards (the
front is the key) – your review history stays.

## Change or add cards

Edit the lists in [`make_deck.py`](make_deck.py), then:

```bash
python3 ~/Documents/University/2026-fall/os/preparation/2-ag-interviews/anki/make_deck.py
```

and import the new `ag-interviews.txt`. If you change a card's *front*, Anki treats it as a new
card (the old one stays – delete it in the browser).
