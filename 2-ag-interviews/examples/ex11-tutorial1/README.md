# ex11 – tutorial 1: a syscall that takes a user string

The first A0 preparation tutorial, **exactly as it was written there**
(`example.patch` = the tutorial's `tutorial-1.patch`). One syscall,
`tutorial_syscall(const char* string)`: the kernel copies the user string (at most 15
characters) into a string member of the calling thread, protected by a Mutex, and prints it
with a debug flag of its own.

The tutorial had no test program. `ex_tutorial1.c` calls the syscall the way a tutor would
test it; `ex_tutorial1_top.c` passes a string that is not terminated before `USER_BREAK`.

```bash
task example ex11      # branch example/ex11-tutorial1 in repos/sweb-ag
task test              # the [TUTORIAL] debug lines, what came back, and the #GF of the second program
```

[A0-11](../../a0/11-thread-note/) is the same task done right.

## The change

| Place | What |
|---|---|
| `common/include/console/debug.h` | `const size_t TUTORIAL = Ansi_Blue \| OUTPUT_ENABLED;` – **your own debug flag**: one line, then `debug(TUTORIAL, ...)` |
| `syscall-definitions.h`, `Syscall.h`, `Syscall.cpp` | `sc_tutorial 1000`, the method, the `case`, the implementation |
| `Thread.h` / `Thread.cpp` | `ustl::string s;` and `Mutex s_mutex;` per thread, the Mutex named in the constructor's initializer list |
| `nonstd.h` / `nonstd.c` | `tutorial_syscall(const char*)` |

```cpp
int Syscall::tutorial(char* string)
{
  debug(TUTORIAL, "%p\n", string);
  if (string == nullptr || (size_t)string >= USER_BREAK)
    return -1U;
  assert(string);
  char temp[16];
  strncpy(temp, string, 15);
  debug(TUTORIAL, "%s\n", string);
  ScopeLock scope_lock(currentThread->s_mutex);
  currentThread->s = temp;
  return 0;
}
```

The three ideas the tutorial shows:

- **A debug flag of your own.** `debug(FLAG, fmt, ...)` prints `[FLAG ]` plus the message to
  the debug console when the flag contains `OUTPUT_ENABLED`. One line in `debug.h` creates
  the flag. Remove `| OUTPUT_ENABLED` to silence it again – the calls can stay.
- **Copy user data into the kernel first** (`temp`), then work with the copy. The kernel may
  read user memory directly: it runs with the process's page tables, and a page that is not
  loaded yet simply page-faults and gets loaded.
- **`ScopeLock`** takes the Mutex in its constructor and releases it in its destructor – at
  the end of the scope, on every `return` path.

## What happens when you run it

```
[TUTORIAL   ]0x8004000
[TUTORIAL   ]hello
[PASS] a normal string: 0
[TUTORIAL   ]0
[INFO] NULL -> 0: the kernel returns -1U, but the case drops the result
[TUTORIAL   ]0x8004130
[TUTORIAL   ]this string is much longer than fifteen characters
...
[TUTORIAL   ]0x7ffffffffffc
[CPU_ERROR  ]#GF: General Protection Fault
[SYSCALL    ]Syscall::EXIT: called, exit_code: 888
```

## What a tutor would ask about this code

| Problem | Consequence | Fix |
|---|---|---|
| `case sc_tutorial: tutorial(...); break;` – no `return_value =` | **every** call returns 0: the `-1U` never reaches user space (the test shows it for NULL and a kernel address) | `return_value = tutorial(...);` |
| `return -1U` | `0xFFFFFFFF`: through an `int` it happens to become -1, through `size_t` it is not -1 | `(size_t) -1` |
| only the **first** byte is checked against `USER_BREAK` | a string that is not terminated before `USER_BREAK`: `strncpy` reads on into `0x800000000000` → General Protection Fault in the kernel, the process is killed (exit 888 – `ex_tutorial1_top`). On a system where the kernel follows directly, it would read kernel memory | check every byte before reading it: copy byte by byte and stop at `USER_BREAK` |
| `strncpy(temp, string, 15)` | if the string has 15 or more characters, `temp` gets **no** terminating NUL (`strncpy` does not add one) – `s = temp` then reads stack garbage until some zero byte | `temp[15] = '\0'` (or `char temp[16] = {}`), or copy byte by byte |
| `debug(TUTORIAL, "%s\n", string)` | prints the **user** string – a second, unbounded read of user memory after the copy (the long string came out in full) | print `temp` |
| `assert(string)` after the NULL check | dead code; asserts are for kernel invariants, not for user input | remove |
| `Mutex s_mutex` per thread | the field is only ever touched by its own thread in its own syscall – the lock protects nothing here. It is needed as soon as **another** thread or kernel path reads or writes `s` | say *which* accesses it orders; a per-thread field that only its own thread uses needs no lock |
| `ustl::string s` | `s = temp` allocates on the kernel heap – fine under a Mutex, never with interrupts off or under a SpinLock in an interrupt path | or `char s[16]`: no allocation |
| `public:` members in `Thread`, named `s` | works; in the team repo per-thread state of user threads belongs into your `UserThread`, with a real name | |
| `sc_tutorial 1000` | fine in base SWEB | your own block in the team repo (1500–1999) |

## Try it

- Make the `case` return the result: what does `tutorial_syscall(0)` return now?
- Change `debug(TUTORIAL, "%s\n", string)` to print `temp` and pass the long string: what
  can appear after the 15 characters? (Whatever follows on the stack – the missing NUL.)
- Remove `| OUTPUT_ENABLED` from the flag: the `[TUTORIAL]` lines disappear, nothing else changes.
