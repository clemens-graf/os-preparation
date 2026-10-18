# A0-14 · string functions

**Time box 20 min · ★☆☆ · type U (user space only) – an `atexit`-style task · read first: [TRIAGE.md](../../TRIAGE.md)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §7 C strings · example: [ex14](../../examples/ex14-libc-function/)

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

## Your team's code

`userspace/libc/include/string.h` (declarations) and `userspace/libc/src/string.c` (the
existing functions – your style guide). **The test does not compile** until the five functions
are declared in `string.h`: that is your first step. No syscall, no kernel – the kernel has its
own `kstring` (`common/source/util/kstring.cpp`), user programs cannot use it.

## Hints

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

## Questions a tutor might ask

- Why is this not a syscall? What would be different if the kernel had to copy a user string?
  (`copyUserString`: check every address against `USER_BREAK`)
- What can go wrong with `strcpy` – and why does `strncpy` not fully fix it?
- Why compare as `unsigned char`?
