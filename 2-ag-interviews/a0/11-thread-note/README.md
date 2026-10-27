# A0-11 · a note per thread (tutorial 1 done right)

**Time box 30 min · ★★☆ · type S ([TRIAGE](../../TRIAGE.md)) · read first: [ex11](../../examples/ex11-tutorial1/) (the tutorial), [ex01](../../examples/ex01-syscalls/)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §15 a thread's life, §7 C strings

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

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

## Level 2 – hints

<details><summary>1 – start from the tutorial</summary>

ex11's syscall is the skeleton: debug flag, the six places, numbers 1570/1571. First fix the
`case`: `return_value = setNote(arg1);` – otherwise nothing you return ever arrives. Return
`(size_t) -1`, never `-1U`.
</details>

<details><summary>2 – where does the note live?</summary>

Per user thread: a member of your `UserThread`. A `char note_[16]` needs no allocation;
initialise it empty in the `UserThread` constructor (the initializer list must follow the
declaration order: `-Werror=reorder`). Not a global – the test notices.
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

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Add a debug flag `NOTE` in `debug.h` and a `char note_[16]` to `UserThread` (empty at start).
2. Syscall plumbing for `setnote` (1570) and `getnote` (1571) – with `return_value =` in both `case`s.
3. `setnote(string)`: refuse 0 and `>= USER_BREAK`. Copy byte by byte into a local 16-byte buffer: before each byte check that its address is below `USER_BREAK`; stop at the NUL or after 15 characters; write the NUL yourself.
4. Copy the local buffer into the thread's `note_`; print it with your debug flag (the copy, not the user string); return the length.
5. `getnote(buffer, size)`: check the buffer range (no overflow); copy at most `size - 1` characters plus a NUL; return the full length of the note.
6. `task test`.

</details>

## Your team's code

The task starts from your team's `main` (`osw26e3`, the commit in `team/BASE`) – the code you have in the interview.

- The note is **per user thread → `UserThread`**: `char note_[16] = {};` (a default member
  initializer – then neither of your two `UserThread` constructors, normal and fork, needs a change).
- In the syscall: `UserThread* thread = (UserThread*) currentThread;` – fine, syscalls only come from
  user threads.
- Your team already has `Syscall::copyUserString(path, buffer, size)` (from `mkdir`). It is **not**
  what setnote needs: it returns `false` when the string is longer than the buffer, setnote must cut
  it after 15 characters. A tutor may ask why you did not reuse it.
- 1570/1571 are free.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>What did the tutorial version return for <code>NULL</code>, and why?</summary>

0 – always. The tutorial's `case` called the function but forgot `return_value = ...`, so the result was dropped and every call looked successful; the error path for `NULL` was invisible.

</details>

<details><summary>Why does <code>strncpy(temp, string, 15)</code> not produce a valid string? What does <code>temp</code> contain then?</summary>

`strncpy(temp, string, 15)` copies at most 15 characters and writes **no** `'\0'` if the source has 15 or more – `temp[15]` is then whatever was on the stack, and using `temp` as a string reads on past its end.

</details>

<details><summary>Why can't you check the string's range before copying, like a buffer?</summary>

A buffer has a known size, so you check `[buffer, buffer + size)` once. A string's length is unknown until you've found its `'\0'` – so you check every address **while** reading, byte by byte, against `USER_BREAK`.

</details>

<details><summary>What happens in the kernel when the user string lies on a page that was not loaded yet? And on an address no segment covers?</summary>

The kernel's read page-faults on a user address – that's legal: the page fault handler maps it (`loadPage`) and the read is repeated. If no segment covers the address, the process is killed (666) – the same as if user code had read it.

</details>

<details><summary>Why copy into a kernel buffer before using it – what could another thread do otherwise?</summary>

Another thread of the process could change (or unmap) the user string between your check and your use (TOCTOU). After copying into a kernel buffer, only the copy is used – nobody else can touch it.

</details>

<details><summary>Do you need a lock for <code>note_</code>? When would you?</summary>

Not as long as only the thread itself reads and writes `note_` in its own syscalls – a thread is never in two syscalls at once. You'd need one (a Mutex in the thread, held while copying in/out) as soon as another thread or kernel path accesses it, e.g. `getnote(tid)` for another thread or a debug dump of all threads.

</details>

<details><summary>How do you add a debug flag, and how do you switch it off again?</summary>

One line in `common/include/console/debug.h`: `const size_t NOTE = Ansi_Cyan | OUTPUT_ENABLED;`, then `debug(NOTE, ...)`. To silence it, remove `| OUTPUT_ENABLED`.

</details>
