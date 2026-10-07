# ex01 – adding syscalls

Two syscalls: `getticks()` returns the timer tick counter (no arguments, just a return
value), `getthreadname(buffer, size)` copies the thread's name into a user buffer.
Test: `ex_syscall.sweb` (4 checks).

## The six places of every syscall

| # | File | Line |
|---|---|---|
| 1 | `common/include/kernel/syscall-definitions.h` | `#define sc_getticks 1400` – the number, shared by kernel **and** libc |
| 2 | `common/include/kernel/Syscall.h` | `static size_t getTicks();` |
| 3 | `common/source/kernel/Syscall.cpp` | `case sc_getticks: return_value = getTicks(); break;` in `syscallException` |
| 4 | `common/source/kernel/Syscall.cpp` | the implementation |
| 5 | `userspace/libc/include/nonstd.h` | `extern size_t getticks(void);` (POSIX-ish things: the matching header, e.g. `pthread.h`) |
| 6 | `userspace/libc/src/nonstd.c` | `return __syscall(sc_getticks, 0x00, 0x00, 0x00, 0x00, 0x00);` |

Plus a test program in `userspace/tests/` – every `.c` there becomes a `.sweb` on the disk
image (after cmake ran again).

## How a syscall travels

```
user: getticks()                         nonstd.c
  → __syscall(nr, a1..a5)                arch/x86/64/userspace/syscalls.c: rax=nr, rbx,rcx,rdx,rsi,rdi = args
  → int $0x80
  → arch_syscallHandler (asm)            arch/x86/64/source/arch_interrupts.S: saves user registers
  → syscallHandler()                     arch/x86/64/source/InterruptUtils.cpp: interrupts ON,
                                         Syscall::syscallException(rax, rbx, rcx, rdx, rsi, rdi)
  → your function                        common/source/kernel/Syscall.cpp
  ← return value written to user_registers_->rax, arch_contextSwitch() back to user
```

## Copying to a user buffer

```cpp
size_t Syscall::getThreadName(size_t user_buffer, size_t size)
{
  if (user_buffer == 0 || size == 0 || size > USER_BREAK || user_buffer >= USER_BREAK - size)
    return (size_t) -1;
  const char* name = currentThread->getName();
  size_t length = ustl::min(strlen(name), size - 1);
  memcpy((char*) user_buffer, name, length);
  ((char*) user_buffer)[length] = '\0';
  return length;
}
```

- **Check the whole range**, written so that it cannot overflow.
- The kernel can access user memory directly: it runs with the process's page tables.
- `(size_t) -1`, never `-1U` (= `0xFFFFFFFF`; user space would not see -1). The test caught
  exactly that bug in the first version of this example.

## Traps found while writing it

- `nonstd.h` does not include `types.h` → `unknown type name 'size_t'`. Add the include.
- Kernel warnings are errors: an unused parameter breaks the build (`-Werror=unused-parameter`).
- Forgotten `break;` → falls into the next case.
