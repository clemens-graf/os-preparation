# A0-14 string functions – solution notes

Patch: [`solution.patch`](solution.patch) – `userspace/libc/include/string.h` (5 declarations) and
`userspace/libc/src/string.c`. Type U: no kernel change.

The solution is written with **indices** (`text[i]`): the pointers never move, only the counter
`i` does. That is the easiest form to write, read and explain. The classic pointer style
(`*d++ = *s++`) does exactly the same – it is shown at the end, for reference.

## The solution (readable version)

```c
// copies source including its NUL
char *strcpy(char *destination, const char *source)
{
  size_t i = 0;
  while (source[i] != '\0')
  {
    destination[i] = source[i];
    i++;
  }
  destination[i] = '\0';                   // the loop stopped before the NUL - copy it too
  return destination;
}

// writes exactly n bytes: the string, then zeros.
// If source has n or more characters, NO terminating NUL is written!
char *strncpy(char *destination, const char *source, size_t n)
{
  size_t i = 0;
  while (i < n && source[i] != '\0')       // copy at most n characters
  {
    destination[i] = source[i];
    i++;
  }
  while (i < n)                            // fill the rest of the n bytes with zeros
  {
    destination[i] = '\0';
    i++;
  }
  return destination;
}

// appends source to the end of destination
char *strcat(char *destination, const char *source)
{
  size_t end = strlen(destination);        // position of destination's NUL
  size_t i = 0;
  while (source[i] != '\0')
  {
    destination[end + i] = source[i];      // overwrite the old NUL, then go on
    i++;
  }
  destination[end + i] = '\0';
  return destination;
}

// compares at most n characters: < 0, 0 or > 0
int strncmp(const char *first, const char *second, size_t n)
{
  for (size_t i = 0; i < n; i++)
  {
    unsigned char a = first[i];            // unsigned: a plain char may be negative
    unsigned char b = second[i];
    if (a != b)
      return a < b ? -1 : 1;
    if (a == '\0')
      return 0;                            // both strings ended here: equal
  }
  return 0;                                // the first n characters are equal
}

// the first position of character in str, or NULL
char *strchr(const char *str, int character)
{
  size_t i = 0;
  while (str[i] != (char) character)
  {
    if (str[i] == '\0')
      return 0;                            // end of the string: not found
    i++;
  }
  return (char *) &str[i];                 // also finds the NUL if character == 0
}
```

## What to say in the interview

- **First step: the declarations** in `string.h` – without them the test does not compile
  (`-Werror=implicit-function-declaration`). Say so: "the header lacks them".
- **`strcpy`:** copies until the NUL and the NUL itself. It cannot know how big `destination`
  is – too small a buffer is a buffer overflow.
- **`strncpy`:** writes exactly `n` bytes. Shorter source → padded with zeros. Source with `n`
  or more characters → **no terminating NUL**. The classic interview question.
- **`strcat`:** find the old end with `strlen`, copy from there – the old NUL gets overwritten.
- **`strncmp`:** must stop at a NUL – comparing beyond it reads memory that is not part of the
  string. `unsigned char`, because characters above 127 would be negative as `char`.
- **`strchr(s, 0)`** returns the address of the NUL, not NULL.
- **`(char *) &str[i]`:** `&str[i]` is the address of slot `i`. The cast removes the `const`
  because POSIX defines the return type as `char *`.
- **Kernel vs. user:** the kernel's `kstring` is a different library in a different address space;
  in the kernel, user strings are copied with `copyUserString` (each byte checked < `USER_BREAK`).

## For reference: the same in classic pointer style

You will see this style in a lot of C code (it comes from the original C book). It is just as
correct – only denser. You do not need to write it this way.

```c
char *strcpy(char *destination, const char *source)
{
  char *d = destination;                   // a second pointer that walks; destination stays at the start
  while ((*d++ = *source++))               // copy one byte, move both pointers, stop after the NUL
    ;
  return destination;
}

char *strncpy(char *destination, const char *source, size_t n)
{
  size_t i = 0;
  for (; i < n && source[i]; ++i)
    destination[i] = source[i];
  for (; i < n; ++i)
    destination[i] = 0;
  return destination;
}

char *strcat(char *destination, const char *source)
{
  strcpy(destination + strlen(destination), source);   // copy to the old end
  return destination;
}

char *strchr(const char *str, int character)
{
  for (;; ++str)                           // str itself walks through the string
  {
    if (*str == (char) character)
      return (char *) str;
    if (*str == 0)
      return 0;
  }
}
```

How to read `*d++ = *source++`, in four steps:

1. `*source` – read the letter where `source` points.
2. `*d = …` – write it where `d` points (the only place the buffer changes).
3. `d++`, `source++` – both pointers move to the next byte.
4. `while (…)` – the value is the copied letter: not 0 → again; `'\0'` → stop. So the NUL is
   copied first and then ends the loop.

| Pointer style | Index style | Meaning |
|---|---|---|
| `char *d = destination;` | `size_t i = 0;` | where the next byte goes |
| `*source` | `source[i]` | the current letter (`source[i]` is `*(source + i)`) |
| `*d = …` | `destination[i] = …` | write it into the buffer |
| `d++`, `source++` | `i++` | next byte |
| `destination + strlen(destination)` | `destination[end + i]` | the old end of the string |
