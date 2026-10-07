# A0-11 · a note per thread (tutorial 1 done right)

**Time box 30 min · ★★☆ · read first: [ex11](../../examples/ex11-tutorial1/) (the tutorial), [ex01](../../examples/ex01-syscalls/)**

```
Name:         setnote, getnote
Nr:           1570 (sc_setnote), 1571 (sc_getnote)
Description:  Every thread has a note of at most 15 characters, empty when the thread starts.
              setnote(string) copies the NUL-terminated user string into the calling
              thread's note; longer strings are cut after 15 characters.
              getnote(buffer, size) copies the note into buffer, NUL-terminated, at most
              size - 1 characters.
Parameters:   const char* string  /  char* buffer, size_t size
Return Value: setnote: the number of characters stored (0..15), or -1 if string is NULL,
              not a user address, or runs into USER_BREAK before its end (or its 15th
              character).
              getnote: the length of the note (also when the buffer was too small), or -1
              if buffer is NULL, size is 0, or [buffer, buffer + size) is not user memory.
Notes:        Print every new note with a debug flag of your own (NOTE). A bad pointer must
              never kill the process or let the kernel read or write beyond USER_BREAK.
```

The tutorial from the first A0 preparation tutorial (ex11) with every gap closed. The
tutorial version returned 0 for every call (the `case` dropped the result), did not
terminate long strings, printed the user string a second time, unbounded, and crashed the
process with a General Protection Fault for a string that runs into `USER_BREAK`.

## Test

`task start a0-11`, implement, `task test`.
Done when all 23 checks pass (`task test` also fails if a test program dies half-way –
it counts the `[PASS]` lines). `note_basic` starts `note_child`: a second process – a second
thread – must see its own, empty note.

## Hints

<details><summary>1 – start from the tutorial</summary>

ex11's syscall is the skeleton: debug flag, the six places, numbers 1570/1571. First fix the
`case`: `return_value = setNote(arg1);` – otherwise nothing you return ever arrives. Return
`(size_t) -1`, never `-1U`.
</details>

<details><summary>2 – where does the note live?</summary>

Per thread: a member of `Thread` (base SWEB; in the team repo your `UserThread`). A `char
note_[16]` needs no allocation; initialise it empty in the constructor (the initializer list
must follow the declaration order: `-Werror=reorder`). Not a global – the test notices.
</details>

<details><summary>3 – reading a string of unknown length</summary>

You cannot check the range up front – you do not know where the string ends. Copy byte by
byte into a kernel buffer and check **each** address before you read it:
`if (string + i >= USER_BREAK) return -1;`, stop at the NUL or after 15 characters, then
write the terminating NUL yourself (`strncpy` does not when the source is long). Read the user
memory directly – a page that is not loaded yet page-faults and is loaded, as always.
Do not use `checkAddressValid`: it says "not mapped" for pages that are just not loaded yet
(a string literal on an untouched page).
</details>

<details><summary>4 – getnote: a buffer of known size</summary>

Exactly ex01's `getthreadname`: `buffer == 0 || size == 0 || size > USER_BREAK ||
buffer > USER_BREAK - size` → `-1` (overflow-safe; a buffer that ends exactly at
`USER_BREAK` is fine). Copy `min(length, size - 1)` characters plus the NUL, return `length`.
</details>

<details><summary>5 – do you need a lock?</summary>

Who reads or writes `note_`? Only the thread itself, in its own syscalls – a thread cannot run
two syscalls at the same time. So: no lock. The tutorial's `Mutex` becomes necessary as soon as
another thread or kernel path touches the field (e.g. "getnote(tid)" for another thread, or a
debug dump of all threads) – be ready to say exactly that.
</details>

## Questions a tutor might ask

- What did the tutorial version return for `NULL`, and why?
- Why does `strncpy(temp, string, 15)` not produce a valid string? What does `temp` contain then?
- Why can't you check the string's range before copying, like a buffer?
- What happens in the kernel when the user string lies on a page that was not loaded yet?
  And on an address no segment covers?
- Why copy into a kernel buffer before using it – what could another thread do otherwise?
- Do you need a lock for `note_`? When would you?
- How do you add a debug flag, and how do you switch it off again?
