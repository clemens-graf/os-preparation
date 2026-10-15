# A0-14 string functions – solution notes

Patch: [`solution.patch`](solution.patch) – `userspace/libc/include/string.h` (5 declarations) and
`userspace/libc/src/string.c`. Type U: no kernel change.

```c
char* strcpy(char* d, const char* s)  { char* p = d; while ((*p++ = *s++)) ; return d; }

char* strncpy(char* d, const char* s, size_t n)
{
  size_t i = 0;
  for (; i < n && s[i]; ++i) d[i] = s[i];
  for (; i < n; ++i) d[i] = 0;          // pad - and no NUL when s has >= n characters
  return d;
}

char* strcat(char* d, const char* s)  { strcpy(d + strlen(d), s); return d; }

int strncmp(const char* a, const char* b, size_t n)
{
  for (size_t i = 0; i < n; ++i)
  {
    unsigned char x = a[i], y = b[i];
    if (x != y) return x < y ? -1 : 1;
    if (x == 0) return 0;               // both end here
  }
  return 0;
}

char* strchr(const char* s, int c)
{
  for (;; ++s) { if (*s == (char) c) return (char*) s; if (*s == 0) return 0; }
}
```

- First step: the declarations – without them the test does not compile
  (`-Werror=implicit-function-declaration`). Say so in the interview: "the header lacks them".
- `strncpy` pads **and** may leave the string unterminated – the classic interview question.
- `strchr(s, 0)` returns the address of the NUL, not NULL.
- `strncmp` must stop at a NUL: comparing beyond it reads memory that is not part of the string.
- Kernel vs. user: the kernel's `kstring` is a different library in a different address space;
  in the kernel, user strings are copied with `copyUserString` (each byte checked < `USER_BREAK`).
