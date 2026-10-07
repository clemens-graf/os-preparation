# A1-10 a page shared by two processes – solution notes

Patch: [`solution.patch`](solution.patch).

## Pieces

| Piece | Where |
|---|---|
| `SharedPages` (key → ppn + reference count), created in `startup()` | new `common/{include,source}/mm/SharedPages.*` |
| `unmapPage(vpn, free_physical_page = true)` | `ArchMemory` |
| `shm_attached_` bit mask per process | `Loader` (under `arch_memory_lock_`) |
| cleanup at exit | `Loader::~Loader` |
| syscalls | `shmAttach`, `shmDetach` |

## Attach / detach

```cpp
// attach: 1. reference (global lock)  2. map (page-table lock) - never both at once
size_t ppn = SharedPages::instance()->acquire(key);        // creates the page if needed
lock arch_memory_lock_; if not yet attached: mapPage + set bit; unlock
if (it was attached already) SharedPages::instance()->release(key);   // keep only one reference

// detach: 1. unmap WITHOUT freeing, flush TLB (page-table lock)  2. release (global lock)
```

## Why each step

- **Reference before mapping:** between the two critical sections another process might release
  its last reference – without ours the page would be freed while we map it.
- **No free in unmap:** the physical page still belongs to the others.
- **Exit without detach:** `~ArchMemory` frees *every* present page. `Loader::~Loader` runs before
  `arch_memory_` (a member) is destroyed: it unmaps the shared pages without freeing them and drops
  the references.
- **Global table:** `SharedPages::init()` in `startup()` – global constructors do not run.

## Locking summary

Two locks, never nested: `SharedPages::lock_` (table + counts) and `Loader::arch_memory_lock_`.

## Variants

Names instead of keys (`shm_open`), a size of n pages, shared between threads of different
processes with a semaphore in the page.
