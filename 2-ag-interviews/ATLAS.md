# Atlas – your team's SWEB, file by file

Where is what, and what happens where – for your team's code (`osw26e3` main, the commit in
[`team/BASE`](team/BASE)). Read section 1 and 2 once completely; use 3–5 to look things up.
Function names instead of line numbers (they move): search them.

**Finding things fast**

| You want | CLion | Terminal (in the repo) |
|---|---|---|
| a file by name | `Ctrl+Shift+N` | `git ls-files \| grep -i stdlib` |
| a function / class | `Ctrl+Alt+Shift+N` (symbol) | `git grep -n "UserProcess::exitThread"` |
| every use of something | `Alt+F7` (find usages) / `Ctrl+Shift+F` | `git grep -n "getThreadLock"` |
| jump to the definition | `Ctrl+B` (or Ctrl+click) | – |
| back to where you were | `Ctrl+Alt+←` | – |
| who calls this function | `Ctrl+Alt+H` (call hierarchy) | `git grep -n "exitProcess("` |

`git grep` searches only tracked files – fast, and never in the build directory.

---

## 1 · The big picture: two worlds and how they meet

```
 USER SPACE  (userspace/)                       KERNEL  (common/, arch/x86/64/)
 ─────────────────────────────────────          ───────────────────────────────────────────
 your test program   userspace/tests/x.c
   │ calls
 libc               userspace/libc/src/*.c      Syscall::syscallException   Syscall.cpp
   │ (some functions are pure C, no kernel!)        ▲
   │ others call                                    │ syscallHandler      InterruptUtils.cpp
 __syscall()  arch/x86/64/userspace/syscalls.c      ▲
   └──── int $0x80 ──────────────────────────────── arch_syscallHandler  arch_interrupts.S
```

The kernel is entered in exactly these ways – every task changes one of them:

| Entry | Triggered by | C function (arch/x86/64/source/InterruptUtils.cpp) | goes on to |
|---|---|---|---|
| syscall | `int $0x80` in `__syscall` | `syscallHandler` | `Syscall::syscallException` (the big switch) |
| page fault | access to an unmapped / protected page | `pageFaultHandler` | `PageFaultHandler::enterPageFault` → `handlePageFault` |
| timer | IRQ 0, many times per second | `irqHandler_0` | `Scheduler::incTicks`, `Scheduler::schedule` (switch thread) |
| keyboard | IRQ 1 | `irqHandler_1` | `KeyboardManager` → `Console::handleKey` (F-keys) |
| CPU error | division by 0, GP fault, … | `errorHandler` | prints, kills the thread |
| `yield` | `Scheduler::yield` | `irqHandler_65` | `Scheduler::schedule` |
| boot | GRUB starts the kernel | `startup()` in `common/source/kernel/main.cpp` | creates everything, starts the shell |

Plus **kernel threads** that run on their own: `Console` (keyboard input), `ProcessRegistry`
(starts the shell, counts processes), `CleanupThread` (deletes dead threads), `IdleThread`.

**Not everything needs the kernel.** Many libc functions are pure C in user space
(`strlen`, `printf` formatting, `atexit`, `malloc` on a static pool, …). Ask first:
*does this need anything only the kernel has or may do?* (→ [TRIAGE.md](TRIAGE.md))

---

## 2 · Call chains – what happens, in which file

### A user program starts
1. Shell types `x.sweb` → libc `createprocess()` (`userspace/libc/src/nonstd.c`) → `sc_createprocess`
2. `Syscall::createprocess` (`Syscall.cpp`) → `ProcessRegistry::createProcess` (`ProcessRegistry.cpp`)
3. `UserProcess::create` (`UserProcess.cpp`): `loadExecutable` (ELF headers → `Loader`),
   `new UserProcess` (new `ArchMemory`), `createThread(entry, …)` → `UserThread` constructor
   (`UserThread.cpp`: user registers with `rip = entry`, stack top, `setAddressSpace`)
4. `startProcess` → `Scheduler::addNewThread` → the timer schedules it
5. First instruction → **page fault** (nothing is loaded yet) → `Loader::loadPage` maps the code page
6. User code starts at **`_start`** (`userspace/libc/src/nonstd.c`): `exit(main());`

### A program ends
```
main returns            → _start: exit(main())                      libc nonstd.c
exit(status)            → __runAtexitFunctions(): atexit, backwards   libc exec.c, stdlib.c
                        → __syscall(sc_exit, status)                 libc exec.c
_exit(status)           → __syscall(sc_exit, status)  (no atexit!)   libc exec.c
Syscall::exit           → process->exitProcess()                     Syscall.cpp
UserProcess::exitProcess→ kills all other threads of the process      UserProcess.cpp
currentThread->kill()   → thread state ToBeDestroyed                  Thread.cpp
CleanupThread           → Scheduler::cleanupDeadThreads → delete thread   Scheduler.cpp
~UserThread             → process->threadExited()                     UserThread.cpp
threadExited            → last thread? delete this                    UserProcess.cpp
~UserProcess            → close fd, delete loader_, delete arch_memory_  UserProcess.cpp
~ArchMemory             → frees every user page + page table          ArchMemory.cpp
```

### A syscall
```
nonstd.c: __syscall(sc_x, a1..a5)   number in rax, args in rbx rcx rdx rsi rdi   syscalls.c
int $0x80 → arch_syscallHandler (saves registers into currentThread->user_registers_)
syscallHandler: interrupts ON, calls Syscall::syscallException(rax, rbx, rcx, rdx, rsi, rdi)
the switch: case sc_x: return_value = x(arg1); break;
return value → user_registers_->rax → back in user space as the return value of __syscall
```

### A page fault
```
pageFaultHandler (InterruptUtils.cpp): error bits → present / user / writing / fetch
PageFaultHandler::enterPageFault → handlePageFault (common/source/mm/PageFaultHandler.cpp)
  checkPageFaultIsValid: user address below USER_BREAK, not present, … ? else → exit(9999)
  valid: thread_lock_ ON
    isUserStackFault  → map stack pages down to the fault (mapStackPage)  → return
    isGuardPage       → exit(9999)
    else: thread_lock_ OFF → getLoader().loadPage(address, getAddressSpace())
          Loader::loadPage: find the ELF segment, allocPPN, copy from the binary, mapPage
          no segment → exit(666)
```

### Timer tick / thread switch
`irqHandler_0` → `Scheduler::incTicks()` (`ticks_`) → `Scheduler::schedule()` picks the next
`schedulable()` thread (round robin, rotates `threads_`) → `arch_contextSwitch` loads its registers.

### A key press
`irqHandler_1` → `KeyboardManager::serviceIRQ` → the `Console` kernel thread → `Console::handleKey`
(`common/source/console/Console.cpp`): F9 bitmap + kernel memory, F10 locks, F11 stack traces,
F12 thread list, Shift+Fn switch terminal. **Free keys: F2–F8** (F1 = terminal 0).
The console thread has the kernel's page tables → set a flag, do the work in the process (ex07).

### pthread_create
```
pthread_create (libc pthread.c) → sc_pthread_create with pthread_start (the libc trampoline),
                                  start_routine, arg
Syscall::pthreadCreate           → checks pointers, process->createThread(entry, true, start, arg)
UserProcess::createThread        → allocateNewStack (stack slot + guard page), new UserThread,
                                   registerThread (UserThreadData in threads_), addNewThread
UserThread constructor           → createUserRegisters(rip = pthread_start), setThreadArguments
                                   (rdi = start_routine, rsi = arg)
pthread_start (libc)             → pthread_exit(start_routine(arg))
```
`pthread_exit` → `UserProcess::exitThread`; `pthread_join` → `joinThread` (waits on
`thread_exited_`); `pthread_cancel` → `cancelThread` (sets `requested_cancel_` + broadcasts,
checked at syscall entry / `pthread_testcancel` / in join's wait loop → `JOIN_CANCELED`);
`pthread_detach` → `detachThread` (`detached_`: `exitThread` deletes the record);
`pthread_equal` → `pthreadEqual` (compares the tids).

### fork / exec
`fork` → `Syscall::fork` → `UserProcess::fork`: `new ArchMemory(parent)` (copy constructor
copies every page), new `UserThread(child, parent)` (copies the registers, `rax = 0` in the child).
`execv` → `Syscall::execv` → `UserProcess::exec` → new `Loader`, fresh address space,
`UserThread::resetForExec`.

---

## 3 · File by file

★ = you will change it in many tasks. **STUB** = exists but does nothing yet (see section 5).

### 3.1 User space – `userspace/`

| File | What is in it |
|---|---|
| `tests/*.c` ★ | one program per file; `name.c` → `/usr/name.sweb`. **New file = new program**, `CMakeLists.txt` picks it up by itself (cmake runs again on the next build). Your team's tests: `pthread_*`, `fork_basic`, `exec_basic`, `mkdir_basic`, … |
| `tests/shell.c` | the shell: reads a line, `createprocess(name)` |
| `libc/include/nonstd.h` ★ | SWEB-only functions: `createprocess`, `getThreadCount`, `getProcessCount` – **your own syscall wrappers go here** |
| `libc/src/nonstd.c` ★ | their implementations (`__syscall(...)`) and **`_start`** (`exit(main())`) – where every program starts |
| `libc/include/stdlib.h` / `src/stdlib.c` ★ | `atexit` (your A0 version: `atexit_functions[ATEXIT_MAX]`, `__runAtexitFunctions`); `malloc`, `free`, `calloc`, `realloc` – **STUBS**; `atoi`/`atol` are in `atoi.c` |
| `libc/src/exec.c` ★ | `exit` (`__runAtexitFunctions()`, then the syscall), `_exit` (syscall only), `fork`, `execv` |
| `libc/include/pthread.h` / `src/pthread.c` ★ | `pthread_create` (+ the trampoline `pthread_start`), `pthread_exit`, `join`, `cancel`, `testcancel`, `self`, `equal`, `detach` work; **STUBS:** `pthread_mutex_*`, `pthread_cond_*`, `pthread_spin_*`, `pthread_setcancelstate/type`. Types: `pthread_t` = `size_t`, `pthread_mutex_t`/`pthread_spinlock_t`/`pthread_cond_t` = `unsigned int` |
| `libc/include/semaphore.h` / `src/semaphore.c` | `sem_init/wait/trywait/post/destroy` – **all STUBS**; `sem_t` = `unsigned int` |
| `libc/src/unistd.c` | `getpid`, `getppid` work; **STUBS:** `brk`, `sbrk`, `sleep`, `ftruncate` |
| `libc/src/time.c` | `clock` – **STUB** (`CLOCKS_PER_SEC` 1000000 in `time.h`) |
| `libc/src/wait.c` | `waitpid` – **STUB** |
| `libc/src/mman.c` | `mmap`, `munmap`, `shm_open`, `shm_unlink`, `mprotect` – **all STUBS** |
| `libc/src/sched.c` | `sched_yield` (syscall 158) |
| `libc/src/string.c` | `memcmp`, `memcpy`, `memset`, `strlen`, `strcmp`, `strstr` – **no `strcpy`/`strncpy`** |
| `libc/src/printf.c` | `printf`, `putchar`, `puts` (formats in user space, then one `write`) |
| `libc/src/scanf.c` | `scanf`, `sscanf`, `getchar`, `gets`, `fgets`, `strtoul`… |
| `libc/src/read.c` / `write.c` / `file_alloc.c` | `read`, `lseek`, `write`; `open`, `close`, `mkdir`, `link`, `unlink`, `pipe`, `dup`… (thin syscall wrappers) |
| `libc/include/types.h` | `size_t`, `ssize_t`, `pid_t`, … – include it where you use `size_t` |
| `../arch/x86/64/userspace/syscalls.c` | `__syscall`: `int $0x80`, number → `rax`, args → `rbx rcx rdx rsi rdi`, result ← `rax` |

### 3.2 Kernel core – `common/source/kernel/` (+ headers in `common/include/kernel/`)

| File | What is in it |
|---|---|
| `Syscall.cpp` ★ | `syscallException` (the switch) and every syscall: `exit`, `write`, `read`, `open`, `close`, `createprocess`, `pthreadCreate/Exit/Join/Cancel/Self/Equal/Detach`, `fork`, `execv`, `getpid`, `mkdir`, `getThreadCount`, `getProcessCount`; helper `copyUserString` (safe user-string copy) |
| `../include/kernel/syscall-definitions.h` ★ | every `sc_…` number – included by kernel **and** libc. Your block 1500–1999 |
| `../include/kernel/Syscall.h` ★ | the declarations (`static size_t x(...)`) |
| `UserProcess.cpp` ★ | **the process**: `create`, `startProcess`, `fork`, `exec`, `createThread`, `registerThread`, `threadExited`; stacks: `allocateNewStack`, `allocateStackPage`, `mapStackPage`, `isGuardPage`; threads: `exitThread`, `joinThread`, `cancelThread`, `detachThread`, `isCancelRequested`, `findThreadData(ByTid)`, `exitProcess`; `getAddressSpace`, `getLoader`, `getThreadLock`; destructor frees loader + address space. **Per-process state goes here** |
| `../include/kernel/UserProcess.h` ★ | `UserThreadData` (`thread_`, `exit_value_`, `tid_`, `exited_`, `joined_`, `requested_cancel_`, `detached_`), `JOIN_CANCELED`, `EXEC_MAX_*`, members: `arch_memory_`, `loader_`, `thread_lock_` (Mutex), `thread_exited_` (Condition), `threads_`, `USERSTACK_PAGES`, `GUARD_PAGE` |
| `UserThread.cpp` ★ | **a user thread**: constructors (new thread / forked thread), `Run`, `resetForExec`, stack top/bottom vpn. **Per-thread state goes into `UserThread.h`**, reach it with `(UserThread*) currentThread` |
| `Thread.cpp` | the base of every thread (kernel and user): kernel stack, `kill`, state, name, TID, terminal, `printBacktrace`; `user_registers_`, `kernel_registers_`, `switch_to_userspace_`, `getProcess()` |
| `Scheduler.cpp` ★ | `schedule` (round robin over `threads_`), `addNewThread`, `yield`, `sleep`/`wake`, `cleanupDeadThreads`, `printThreadList`, `getTicks`/`incTicks`, `getThreadCount`, `getNextTid`. **Scheduling tasks change `schedule()`** |
| `Loader.cpp` | the ELF binary of a process: `readHeaders`, `getEntryFunction`, **`loadPage`** (lazy loading of code/data; exit 666 if no segment) |
| `ProcessRegistry.cpp` | kernel thread: mounts `/usr`, starts the programs of `user_progs.h` (the shell), `createProcess`, counts processes (`processStart/Exit/Count`), `registerPID` |
| `Mutex.cpp`, `SpinLock.cpp`, `Condition.cpp`, `Lock.cpp`, `ScopeLock.h` | kernel locks with checks (deadlock detection, wrong release). Mutex sleeps, SpinLock spins (no `new`/`delete` while holding one) |
| `CleanupThread.cpp`, `IdleThread.cpp` | the two helper kernel threads |
| `main.cpp` | `startup()`: boot order (PageManager, console, scheduler, interrupts, file systems, block devices, timer, keyboard, kernel threads). **Kernel-wide objects / a new kernel thread: create them here** |
| `../include/kernel/user_progs.h` | the programs started at boot (only the shell – don't rename the variable) |

### 3.3 Memory – `common/source/mm/` and `arch/x86/64/`

| File | What is in it |
|---|---|
| `mm/PageFaultHandler.cpp` ★ | `checkPageFaultIsValid`, `isUserStackFault`, **`handlePageFault`** (stack growth, guard page, `loadPage`), `enterPageFault`. **Heap / magic page / swap-in / copy-on-write go here** |
| `mm/PageManager.cpp` ★ | physical pages: `allocPPN` (zeroed), `freePPN`, `getTotalNumPages`, `getNumFreePages`, `printBitmap` (F9) |
| `mm/KernelMemoryManager.cpp` | the kernel heap behind `new`/`delete`/`kmalloc` |
| `mm/new.cpp`, `kmalloc.cpp` | `operator new/delete`, `kmalloc/kfree` |
| `arch/x86/64/source/ArchMemory.cpp` ★ | page tables: `mapPage` (`[[nodiscard]]`), `unmapPage` (frees the page + empty tables), `resolveMapping`, `checkAddressValid`, `flushTlb`, `getIdentAddressOfPPN`, copy constructor (fork), destructor (frees all user pages) |
| `arch/x86/64/include/ArchMemory.h` ★ | `ArchMemoryMapping` (`page_size`, `page_ppn`, `pt_ppn`, …); add new methods in the `public:` part |
| `arch/x86/64/include/paging-definitions.h` | `PAGE_SIZE`, the entry bit fields: `present`, `writeable`, `user_access`, `accessed`, `dirty`, `ignored_*` (free bits for you), `execution_disabled` |
| `arch/x86/64/include/offsets.h` | `USER_BREAK` (0x800000000000), `KERNEL_START`, `IDENT_MAPPING_START` |

### 3.4 Interrupts, registers, console

| File | What is in it |
|---|---|
| `arch/x86/64/source/InterruptUtils.cpp` ★ | the C side of every interrupt: `syscallHandler`, `pageFaultHandler`, `irqHandler_0` (timer), `irqHandler_1` (keyboard), `errorHandler`; `countPageFault` (halts after 10 identical faults) |
| `arch/x86/64/source/arch_interrupts.S` | assembly stubs: save registers, call the C handler, `iretq` |
| `arch/x86/64/source/ArchThreads.cpp` | `createUserRegisters`, `createKernelRegisters`, `setThreadArguments` (rdi/rsi), `changeInstructionPointer`, `setAddressSpace` (cr3), atomics: `testSetLock`, `atomic_add`, `atomic_set` |
| `arch/x86/64/include/ArchThreads.h` | `struct ArchThreadRegisters`: `rip`, `rsp`, `rax`, `rbx`, `rcx`, `rdx`, `rsi`, `rdi`, `rflags`, `cr3`, `fpu`, … |
| `arch/x86/64/source/ArchInterrupts.cpp` | timer frequency, enable/disable interrupts, timer, keyboard |
| `common/source/console/Console.cpp` ★ | `handleKey` – the F-key switch (add `case KEY_F7:` …) |
| `common/source/console/kprintf.cpp` | `kprintf` (screen), `kprintfd` (debug output only) |
| `common/include/console/debug.h` ★ | debug flags: `const size_t NAME = Ansi_Cyan \| OUTPUT_ENABLED;` then `debug(NAME, "...")` |

### 3.5 Block devices and files (A2 swap tasks)

| File | What is in it |
|---|---|
| `common/source/fs/BDManager.cpp` | finds the disks: `BDManager::getInstance()->getDeviceByNumber(n)` / `device_list_` (`idea0`, `idea1` = `/usr`, `idea2` = swap) |
| `common/source/fs/BDVirtualDevice.cpp` | `readData(offset, size, buffer)`, `writeData(...)`, `getBlockSize()` |
| `common/source/fs/VfsSyscall.cpp` | the file system behind `open/read/write/mkdir` |

### 3.6 Build

| File | What is in it |
|---|---|
| `CMakeLists.txt` (top) | the build; `make qemu` / `make qemugdb` targets |
| `userspace/tests/CMakeLists.txt` | builds every `*.c` in `tests/` into a `.sweb` – nothing to add for a new test |
| `utils/` | tools of the build (e.g. `exe2minixfs` puts the programs on the disk image) |

---

## 4 · "I want to … " → go to

| I want to … | File → function |
|---|---|
| add a syscall | the six places: `syscall-definitions.h`, `Syscall.h`, `Syscall.cpp` (case + function), `nonstd.h`, `nonstd.c`, a test |
| implement a POSIX function that is a stub | its libc file (section 3.1) – first check if it needs the kernel ([TRIAGE.md](TRIAGE.md)) |
| run code when a program exits | user side: `exit()` in `libc/src/exec.c`; kernel side: `Syscall::exit` / `UserProcess::exitProcess` |
| run code when a process is deleted | `~UserProcess` (`UserProcess.cpp`) – the last thread is gone |
| run code when a program starts | user side: `_start` in `nonstd.c`; kernel side: `UserProcess::create` / `startProcess` |
| store something per process | member in `UserProcess.h` (+ init in the constructor's initializer list or `= 0`) |
| store something per thread | member in `UserThread.h`, reach it via `(UserThread*) currentThread` |
| store something global in the kernel | a `static` member (e.g. `Syscall::x_`) or a global in a `.cpp`; create objects in `main.cpp` |
| get the calling process / thread | `currentThread->getProcess()` / `(UserThread*) currentThread` |
| find a thread by TID | `process->findThreadDataByTid(tid)` (hold `getThreadLock()`!) |
| map / unmap a page | `process->getAddressSpace().mapPage(vpn, ppn, 1)` / `.unmapPage(vpn)` + `ArchMemory::flushTlb()` |
| find out what is mapped | `getAddressSpace().resolveMapping(vpn)` → `.page_size`, `.page_ppn` |
| read/write a physical page from the kernel | `(char*) ArchMemory::getIdentAddressOfPPN(ppn)` |
| get / free a physical page | `PageManager::instance()->allocPPN()` / `freePPN(ppn)` |
| react to a page fault | `handlePageFault` in `PageFaultHandler.cpp` (new case first in the valid branch) |
| change how pages are loaded from the binary | `Loader::loadPage` |
| change a page's flags (read-only, NX, …) | `ArchMemory.cpp` + the bit fields in `paging-definitions.h` |
| count something every timer tick | `irqHandler_0` (`InterruptUtils.cpp`) or `Scheduler::incTicks` |
| change which thread runs next | `Scheduler::schedule` |
| make a thread sleep / wake up | kernel: `Condition::wait` / `signal` (with its Mutex), `Scheduler::sleep` / `wake` |
| react to a key | `Console::handleKey` (`case KEY_F7:`) – set a flag, do the work in the process |
| start a kernel thread | a class derived from `Thread` with `Run()`, `Scheduler::instance()->addNewThread(new X)` in `main.cpp` |
| set where a user thread continues | `currentThread->user_registers_->rip` (args: `rdi`, `rsi`, `rdx`; stack `rsp`) |
| copy a string from user space | `Syscall::copyUserString(user_ptr, buffer, size)` |
| check a user pointer | `ptr != 0 && size <= USER_BREAK && ptr <= USER_BREAK - size` |
| print kernel debug output | `debug(FLAG, "...")` (flags in `debug.h`) or `kprintfd` |
| print every thread | F12 → `Scheduler::printThreadList` |
| read/write the swap disk | `BDManager::getInstance()->getDeviceByNumber(...)` → `readData` / `writeData` |
| know the time | `Scheduler::instance()->getTicks()` (timer ticks since boot) |
| use an atomic in the kernel | `ArchThreads::testSetLock`, `atomic_add`, `atomic_set` |
| use an atomic in user space | `__sync_lock_test_and_set(&x, 1)`, `__sync_lock_release(&x)`, `__sync_fetch_and_add(&x, 1)` (GCC builtins, no include) |

---

## 5 · POSIX functions that are stubs in your libc

They exist (header + function), but return an error or do nothing. A task like "implement
`malloc`" means: fill in the stub. The **type** says where the work is ([TRIAGE.md](TRIAGE.md)):
**U** = only user space, **S** = needs a new syscall + kernel code.

| Function(s) | File | Type | The simple, working way | Course task |
|---|---|---|---|---|
| `atexit` | `stdlib.c` + `exit` in `exec.c` | U | **done** – your A0 version is merged: array of 32 function pointers + counter; `exit` calls them backwards, then the syscall | A0-13 (study) |
| `strcpy`, `strncpy`, `strcat`, `strncmp`, `strchr` | `string.c` (+ declarations in `string.h`) | U | **missing**, not even stubs – plain C loops | A0-14 |
| `malloc`, `free`, `calloc`, `realloc` | `stdlib.c` | U | a big static array as pool, a small header (size + used) before every block, first fit; `free` sets used = 0 | A1-14 |
| `pthread_spin_init/lock/trylock/unlock` | `pthread.c` | U | `while (__sync_lock_test_and_set(lock, 1)) sched_yield();` – the one place you need an atomic | A1-15 |
| `pthread_mutex_*` | `pthread.c` | U | the same as the spinlock (a mutex that yields while it waits) | A1-15 |
| `sem_init/wait/trywait/post` | `semaphore.c` | U | one global spinlock (A1-15) protects the counter; `sem_wait` loops: lock, value > 0 → value--, unlock, else unlock + `sched_yield` | A1-16 |
| `pthread_cond_*` | `pthread.c` | U (simple) / S (real) | simple: a generation counter – `wait` unlocks, yields until the counter changed, locks again; `signal`/`broadcast` increment it | A1-17 |
| `pthread_detach` | `pthread.c` | S | **done** – yours, merged: a `detached_` flag in `UserThreadData`; `exitThread` frees the data at once, `join` refuses | A1-20 (study) |
| `sleep` | `unistd.c` | S | kernel: wait until `Scheduler::getTicks()` reached start + seconds × tick rate (yield in a loop) | A1-18 |
| `clock` | `time.c` | S | a syscall that returns ticks of the process | A1-07 |
| `sbrk`, `brk` | `unistd.c` | S | per-process break in `UserProcess`, pages mapped on demand in the page fault handler | A1-12 |
| `waitpid` | `wait.c` | S | `ProcessRegistry` remembers exit codes per pid; the caller waits on a Condition | A1-19 |
| `mmap`, `munmap`, `shm_*` | `mman.c` | S | like A0-02 (allocpage) with a size; shared pages like A1-10 | A2-13 (mmap), A1-10 (shared) |
| `mprotect` | `mman.c` | S | change `writeable` / `execution_disabled` bits + `flushTlb` | A2-07 |
| `pthread_setcancelstate/type` | `pthread.c` | S | a flag per thread in `UserThreadData`, checked where `requested_cancel_` is checked | – |
