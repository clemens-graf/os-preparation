# A0-14 · string functions

**Time box 20 min · ★☆☆ · type U (user space only) – an `atexit`-style task · read first: [TRIAGE.md](../../TRIAGE.md)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §7 C strings · example: [ex14](../../examples/ex14-libc-function/)

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         strcpy, strncpy, strcat, strncmp, strchr (string.h)
Description:  SWEB's libc has strlen, strcmp, strstr, memcpy, memset - but not these.
              Add them as in POSIX:
              strcpy(d, s)       copies s including its NUL, returns d
              strncpy(d, s, n)   copies at most n bytes; if s is shorter, fills the rest of
                                 the n bytes with 0; if s is longer: NO terminating NUL
              strcat(d, s)       appends s to the end of d, returns d
              strncmp(a, b, n)   compares at most n characters (<0, 0, >0), stops at a NUL
              strchr(s, c)       the first c in s (also the NUL if c == 0), else NULL
Return Value: as above
```

## Test

`task start a0-14` → `task test` (`strings_basic.sweb`, then `exit`). Done when 9 checks pass.

## Level 2 – hints

<details><summary>1 – where does it go?</summary>

Type U: nothing here needs the kernel. `git grep -n strstr userspace/libc` shows you both files.
</details>

<details><summary>2 – the strncpy trap</summary>

It writes **exactly n** bytes: the string, then zeros up to n. A source of n or more characters
gets no terminating NUL – that is POSIX, and the test checks that you do not write byte n+1.
</details>

<details><summary>3 – strncmp: compare as unsigned char</summary>

Stop at the first difference **or** at a NUL in both. Compare `(unsigned char)` values – `char`
may be signed, and `'\xe4' < 'a'` would be wrong.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Open `userspace/libc/include/string.h` and add the five **declarations** (`strcpy`, `strncpy`, `strcat`, `strncmp`, `strchr`) next to the existing `strlen`/`strcmp` – without them the test does not compile.
2. Open `userspace/libc/src/string.c`; write each function with an **index** `i` that counts up (the pointers stay where they are).
3. `strcpy`: copy character `i` while it is not the NUL; after the loop write the NUL yourself; return the destination.
4. `strncpy`: copy while `i < n` **and** not the NUL; then fill with zeros until `i == n`. If the source was long, no NUL is written – that is correct.
5. `strcat`: find the end of the destination (its length), then copy the source starting there, NUL included.
6. `strncmp`: compare at most `n` characters; stop when they differ or at a NUL; return the difference of the two characters **as `unsigned char`**; 0 if all were equal.
7. `strchr`: walk until you find the character (return its address) or the NUL; a search for `'\0'` returns the address of the NUL; otherwise NULL.
8. `task test` – then check each failing line against the card's exact rules.

</details>

## Your team's code

`userspace/libc/include/string.h` (declarations) and `userspace/libc/src/string.c` (the
existing functions – your style guide). **The test does not compile** until the five functions
are declared in `string.h`: that is your first step. No syscall, no kernel – the kernel has its
own `kstring` (`common/source/util/kstring.cpp`), user programs cannot use it.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why is this not a syscall? What would be different if the kernel had to copy a user string? (<code>copyUserString</code>: check every address against <code>USER_BREAK</code>)</summary>

The function only reads and writes the process's own memory – nothing only the kernel has. If the **kernel** had to copy a user string, it couldn't use `strcpy`: the length is unknown and every byte must be checked against `USER_BREAK` (`copyUserString`), or an unterminated string would make the kernel read kernel memory.

</details>

<details><summary>What can go wrong with <code>strcpy</code> – and why does <code>strncpy</code> not fully fix it?</summary>

`strcpy` writes until the source's `'\0'` – a longer source overflows the destination (buffer overflow). `strncpy(d, s, n)` limits the bytes, but when `s` has `n` or more characters it writes **no** `'\0'` – the result is not a string, and the next string function reads on past the end.

</details>

<details><summary>Why compare as <code>unsigned char</code>?</summary>

Characters above 127 are negative in a plain (signed) `char` on x86. Comparing them as signed would sort `'é'` *before* `'a'`. POSIX says the difference is computed as `unsigned char` – then byte values 0–255 compare like the numbers they are.

</details>
