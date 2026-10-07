# Module 11 — SWEB bridge worksheet

Everything from modules 00–10, now in the real code. Work through it with
your SWEB checkout open next to it (paths below are relative to
`repos/osw26e3`). Write your answers down: parts 3 and 4 are the input for
**Phase 0 (1–4 Oct)** and for your **P1 task (`pthread_create` from 5 Oct,
`pthread_join` from 21 Oct)**.

Reference answers: `../solutions/11-sweb-bridge/answers.md` — look only
after you have your own.

> **Do not experiment in your group repository.** For the hands-on parts
> use a throwaway copy:
> ```bash
> git clone ~/Documents/University/2026-fall/os/repos/osw26e3 ~/sweb-playground
> ```
> Your group's `main` gets the Phase 0 commit you make *together*.

---

## Part 0 — Build, boot, observe (≈ 45 min)

**Build.** From CLion: *Reload CMake Project*, build, run the `qemu`
target. From a terminal (a build directory *outside* the source tree;
SWEB's own script `setup_cmake.sh` defaults to `/tmp/sweb`):
```bash
cmake -B /tmp/sweb -S ~/sweb-playground
cmake --build /tmp/sweb -j
cd /tmp/sweb && make qemu
```
(`cmake` is not installed system-wide on this machine, but CLion's is:
`~/.local/share/JetBrains/Toolbox/apps/clion/bin/cmake/linux/x64/bin/cmake`.)

`make qemu` opens a window for the SWEB console. Everything the kernel
prints with `debug(...)`/`kprintfd` goes to the **debug console**
(`-debugcon stdio`): your terminal, and `output.log` in the build
directory.

**Headless.** `tools/sweb_run.py` boots a build without a window, presses
Enter at the GRUB menu (it has no timeout), types commands into the SWEB
shell and prints the debug output of each one. Exit status 0 = the shell
prompt came back, 1 = timeout (a hang!), 2 = kernel panic, 3 = no boot.
```bash
tools/sweb_run.py -b /tmp/sweb help mult.sweb        # from preparations/11-sweb-bridge
tools/sweb_run.py -b /tmp/sweb -n 10 -t 30 my_test.sweb   # ten boots in a row
```
The `-n` option is for the A1 rule of thumb: *races appear on the tenth
run, not the first.*

**Tasks.**
1. Build the playground and run `mult.sweb` (window or headless). Where in
   the debug output do you find its result, and how did it get there?
   (The comment in `userspace/tests/mult.c` tells you the expected value.)
2. The debug output is dominated by `[SYSCALL]` lines. Find where the
   debug categories are defined and silence exactly that one.
3. Add a user program `userspace/tests/hello.c` that prints one line.
   What must you do so that `hello.sweb` appears on the disk image?
   (Hint: the programs are collected with a CMake `file(GLOB ...)`.)
   If SWEB then fails with *"Error: loading /usr/shell.sweb failed!"*,
   build once more: a parallel build can copy the programs onto the
   image before the new one is linked.
4. Find the gdb targets in `CMakeLists.txt` / `arch/x86/64/CMakeLists.include`.
   How do you stop SWEB at `Syscall::write` with gdb?

---

## Part 1 — Where is what? (≈ 30 min)

Fill in the file for each item (a `grep -rn` or CLion's *Go to Symbol*
finds everything):

| What | File(s) |
|---|---|
| the kernel's C++ entry point after boot | |
| the system call dispatcher | |
| the user-side `__syscall` (x86-64) | |
| syscall numbers, shared by kernel and libc | |
| the libc `pthread_*` stubs you will implement | |
| `Thread`, its states, its kernel stack | |
| `UserProcess` — how is it related to `Thread`? | |
| the scheduler and its thread list | |
| the thread that deletes dead threads | |
| page tables: map / unmap / resolve | |
| the page-fault handler and the loader's `loadPage` | |
| the frame allocator | |
| `Mutex`, `Condition`, `SpinLock`, the common `Lock` base | |
| `USER_BREAK` and the kernel's identity mapping | |
| the first user program the kernel starts | |

---

## Part 2 — Six traces (≈ 2 h)

Follow each path through the code with the reading questions. Each one
repeats a module; the module's answers are a good check.

**T1 — Boot to shell** (modules 01, 08). Start at `startup()` in
`common/source/kernel/main.cpp`.
- Which kernel threads exist before the first user program runs?
- `ProcessRegistry::createProcess` → `new UserProcess(...)`: what does the
  constructor load, which single page does it map, and where does the new
  thread's user stack pointer point?
- What is the first user instruction executed (`loader_->getEntryFunction()`
  and `userspace/libc/src/nonstd.c`)? What happens when `main` returns?

**T2 — One system call** (module 07). `write(1, "hi", 2)` in a user
program.
- Name every function from `userspace/libc/src/write.c` to `kprintf` and
  back. Where do the user registers get saved, where is the return value
  written?
- Which registers carry the syscall number and the arguments? Which
  argument register would a sixth argument need?
- Which user-pointer check does `Syscall::write` perform, and which one
  does it get wrong?

**T3 — Timer and scheduler** (module 09).
- `irqHandler_0` → `Scheduler::schedule()`: policy, quantum, what happens
  to a `Sleeping` thread?
- `Scheduler::yield()` → `int $65`: how does a voluntary switch differ
  from a preemption?
- `Scheduler::sleep()` / `wake()`: why does `wake` yield in a loop?

**T4 — A page fault** (module 08).
- `PageFaultHandler::enterPageFault` → `checkPageFaultIsValid` →
  `Loader::loadPage` → `ArchMemory::mapPage`. Which faults are accepted,
  which kill the process?
- A user program touches the page *below* its stack page. What happens,
  and which line decides it?
- Two threads of one process fault at the same time (after A1). Which
  data is shared, which lock protects it — and which does not exist?

**T5 — The death of a thread** (module 10).
- `Syscall::exit` → `Thread::kill()` → ... → `Scheduler::cleanupDeadThreads`
  → `delete`. Why is the deletion deferred to another thread?
- What does `~UserProcess` delete? Imagine a second thread of the same
  process still running at that moment.
- Run the playground's `mult.sweb` and find this sequence in the debug
  output.

**T6 — Locks** (modules 04, 05, 06).
- `Mutex::acquire`: spin or sleep? Is there barging (module 10, Q5)?
- `Condition::wait` → `Lock::sleepAndRelease`: why can the wake-up not
  get lost although the waiters list is unlocked *before* sleeping?
- What do `holding_lock_list_` and `lock_waiting_on_` enable
  (module 06, Q7)? What does `Condition::wait` assert about other held
  locks, and why?

---

## Part 3 — Phase 0: split `UserProcess` from `Thread` (≈ 1.5 h)

In baseline SWEB a process *is* a thread (`class UserProcess : public
Thread`). The A1 plan's definition of done: *a process owns the address
space, file descriptors and PID; a thread owns stack, registers and TID;
SWEB still boots and runs a user program.*

1. **Who owns what?** List every data member of `Thread` and
   `UserProcess` (and the `Loader` it owns) and decide: process or
   thread? Where it is not obvious (terminal, working directory, name,
   `switch_to_userspace_`, the lock lists), write down why.
2. **Thread IDs.** Build a playground program that prints its thread id
   (you need a tiny system call for that — see part 5). What does it print,
   and why? Where must ids (and process ids) come from, and who hands them
   out under which lock?
3. **Finding the process.** Kernel code reaches the address space through
   `currentThread->loader_` in many places. Count the uses (`grep -rn "loader_" common arch | wc -l`).
   How will a thread find its process after the split, and which of these
   uses must change?
4. **Lifetime.** Today `~UserProcess` deletes the `Loader` (address
   space). After the split, when may the process — and its address space —
   be destroyed? Who destroys it, on which stack, and how does it know
   that its last thread is gone?
5. **Counting.** `ProcessRegistry` counts processes to know when to
   unmount and shut down. What must it count after the split?
6. **Exit.** What does `Syscall::exit` do with one thread today, and what
   must it do in a process with several threads (module 07, Q7)?
7. **Sketch** the new classes (members, constructor arguments), and list
   the files that change. Keep it to one page: this is what you bring to
   the Phase 0 meeting.

---

## Part 4 — P1: `pthread_create` and `pthread_join` (≈ 1.5 h)

1. **The libc side.** Read `userspace/libc/src/pthread.c` and
   `userspace/libc/include/pthread.h`. What must the user-space
   `pthread_create` pass to the kernel so that a *return* from
   `start_routine` ends the thread properly (module 10, Q1)?
2. **The system call.** Choose a number and write down the signature of
   the kernel function. For each argument: user pointer or not, which
   check, when (module 07, Q9). What do you return on error?
3. **The new thread.** Read `ArchThreads::createUserRegisters` and
   `createBaseThreadRegisters`. Which register values must the new thread
   get (`rip`, `rsp`, the argument registers, `cr3`), and which helper
   sets `cr3`?
4. **Its user stack.** Where does it go (not on top of the first thread's
   stack page at `USER_BREAK - PAGE_SIZE`), how big, how is it mapped —
   eagerly, or lazily by a changed page-fault path (T4, module 08 Q9)? How
   is a stack slot reused after the thread has been joined?
5. **Join.** Which per-thread data must survive the thread's death until
   the join? Where do you keep it? How does the joiner wait — which SWEB
   primitive, which lock, which condition (module 09 Q8)? Which errors
   (module 10, part A)?
6. **Tests first.** Write down ten test programs for `userspace/tests/`
   that together prove the definition of done in the A1 plan (*"a user
   program starts a second thread that runs the given function with its
   argument; both threads are scheduled and the process survives both"*;
   for join: *"the joiner blocks until the target finishes and receives
   exactly the value exit stored; a detached thread cannot be joined"*).
   Include corner cases: many threads, a thread creating threads, main
   exiting first, a bad pointer for `thread`, joining yourself, joining
   twice. Run them with `sweb_run.py -n 10`.

---

## Part 5 — Warm-up: your first system call (optional, ≈ 45 min, playground only)

Add `gettid()` end to end — the same steps `pthread_create` will need:

1. `common/include/kernel/syscall-definitions.h`: a new number
   (`sc_gettid`) that is not used yet.
2. `common/include/kernel/Syscall.h` / `common/source/kernel/Syscall.cpp`:
   a `static size_t gettid()` returning `currentThread->getTID()`, and a
   `case` in `Syscall::syscallException`.
3. `userspace/libc/include/nonstd.h` / `userspace/libc/src/nonstd.c`: a
   wrapper `int gettid(void)` calling `__syscall(sc_gettid, 0, 0, 0, 0, 0)`.
4. `userspace/tests/tid.c`: print the result.
5. Reload CMake, build (twice, see part 0), run `tid.sweb`.

Then answer part 3, question 2.
