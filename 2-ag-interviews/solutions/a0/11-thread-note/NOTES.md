# A0-11 a note per thread – solution notes

Patch: [`solution.patch`](solution.patch) (against upstream `f6fcb2ab`, tests excluded).
Starting point: the tutorial version, [ex11](../../../examples/ex11-tutorial1/).

## Pieces

| Piece | Where |
|---|---|
| debug flag `NOTE = Ansi_Cyan \| OUTPUT_ENABLED` | `common/include/console/debug.h` |
| `char note_[16]`, initialised empty | `Thread.h`, `Thread.cpp` (initializer list, in declaration order) |
| syscall numbers 1570/1571 | `syscall-definitions.h` |
| `setNote`, `getNote` + two `case`s **with** `return_value =` | `Syscall.h`, `Syscall.cpp` |
| wrappers, `#include "types.h"` | `nonstd.h`, `nonstd.c` |

## setnote: a string of unknown length

```cpp
size_t Syscall::setNote(size_t string)
{
  if (string == 0 || string >= USER_BREAK)
    return (size_t) -1;
  char note[sizeof(currentThread->note_)];
  size_t length = 0;
  while (length < sizeof(note) - 1)
  {
    if (string + length >= USER_BREAK)
      return (size_t) -1;                       // runs into the kernel half before its NUL
    char c = ((const char*) string)[length];    // may page-fault: loads a lazy user page
    if (c == '\0')
      break;
    note[length++] = c;
  }
  note[length] = '\0';
  memcpy(currentThread->note_, note, sizeof(note));
  debug(NOTE, "%s: note is now \"%s\"\n", currentThread->getName(), note);
  return length;
}
```

## getnote: a buffer of known size

```cpp
if (buffer == 0 || size == 0 || size > USER_BREAK || buffer > USER_BREAK - size)
  return (size_t) -1;
size_t length = strlen(currentThread->note_);
size_t n = ustl::min(length, size - 1);
memcpy((char*) buffer, currentThread->note_, n);
((char*) buffer)[n] = '\0';
return length;
```

## Why each step

- **`return_value = setNote(...)`:** the tutorial's `case` called the function and dropped the
  result – every call returned 0, the error path was invisible.
- **Byte by byte, each address checked:** the end of a string is unknown, so there is no range
  to check up front. The tutorial checked only the first byte; a string that runs into
  `USER_BREAK` made `strncpy` read `0x800000000000` → General Protection Fault in the kernel,
  the process killed (exit 888). `string < USER_BREAK` and `length <= 15` make
  `string + length` overflow-free.
- **The NUL written by hand:** `strncpy(dst, src, 15)` copies 15 characters and *no* NUL when
  the source is longer – the tutorial's `s = temp` then read on into the stack.
- **Read user memory directly, not via `checkAddressValid`:** the kernel runs with the process's
  page tables; an unloaded user page page-faults and the Loader maps it – exactly as for the
  program itself. `checkAddressValid` would wrongly refuse a string on a page that is merely not
  loaded yet. An address that no segment covers still kills the process (exit 666), as
  everywhere in SWEB – the card only promises safety at `USER_BREAK`.
- **Copy first, then use:** after the loop only the kernel copy is used (and printed). The
  tutorial printed the *user* string again – a second, unbounded read that another thread
  could have changed in between.
- **`char note_[16]` instead of `ustl::string`:** no heap allocation in the syscall; the size
  limit is part of the type.
- **`getnote` returns the full length:** like `snprintf`, the caller can tell that its buffer
  was too small.

## Locking

None – and that is a statement you must be able to defend: `note_` is read and written only
by its own thread, in its own syscalls, and one thread is never in two syscalls at once. The
tutorial's per-thread `Mutex` would be needed as soon as another thread or kernel path accesses
the field (a `getnote(tid)` for another thread, a debug dump of all threads, the F-key handler
printing the thread list). Then: a Mutex in the thread, held while copying in or out, and
nothing that can sleep or allocate under a SpinLock.

## Pitfalls

- `-Werror=reorder`: `note_` is declared after `holding_lock_list_`, so `note_()` goes right
  after `holding_lock_list_(0)` in the initializer list.
- The exact-fit buffer: `buffer >= USER_BREAK - size` refuses a buffer that ends exactly at
  `USER_BREAK` – the test checks that boundary (`>` is right).
- `nonstd.h` needs `types.h` for `size_t`.

## Answers to the card's questions

- **NULL in the tutorial version:** 0 – the result was dropped in the `case`.
- **`strncpy`:** no NUL when the source has 15 or more characters; the rest of `temp` is
  whatever was on the stack.
- **Range up front:** the length is unknown until you have read up to the NUL.
- **Unloaded page:** a page fault in kernel mode on a user address – legal, the Loader maps the
  page; an address without a segment: the process is killed (`No section refers to the given
  address`, exit 666).
- **Kernel copy:** another thread can change the user string between check and use (TOCTOU).
- **Lock:** see "Locking".
- **Debug flag:** one line in `debug.h`; remove `| OUTPUT_ENABLED` to silence it.
