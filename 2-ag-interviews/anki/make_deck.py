#!/usr/bin/env python3
"""make_deck.py - writes ag-interviews.txt, an Anki import file (File -> Import).

All cards are about your team's code (osw26e3, the commit in ../team/BASE).
Edit the lists below and run:  python3 make_deck.py
Re-importing updates the cards (the first field is the key): your review history is kept.
"""

import html
import os

ROOT = "OS AG"


def c(text):
    """inline code"""
    return "<code>" + html.escape(text) + "</code>"


def pre(text):
    """a code block"""
    return "<pre>" + html.escape(text.strip("\n")) + "</pre>"


# (front, back, tags) per sub-deck. Fronts must be unique.
DECKS = {}

# --------------------------------------------------------------------------------------------
DECKS["1 Triage"] = [
    ("The five task types (TRIAGE.md)?",
     "<b>U</b> user space only (libc) · <b>S</b> syscall + kernel · <b>E</b> kernel event "
     "(page fault, timer, scheduler, key, boot, exit) · <b>A</b> assembly / registers · "
     "<b>K</b> the kernel calls a user function", "triage"),
    ("The one question that decides U vs. S?",
     "Does it need anything <i>only the kernel</i> has or may do? (page tables, physical pages, "
     "other threads/processes, devices, the scheduler, the time) – no → libc only.", "triage"),
    ("The first five minutes of an unknown task?",
     "1. read the box twice · 2. type (U/S/E/A/K) · 3. one search for the starting point · "
     "4. where does the state live? · 5. who touches it at the same time (lock)? · "
     "6. write the 3–6 files down, then plumbing → build → logic → test. Say it out loud.", "triage"),
    ("Task: \"implement atexit\" – type, first file?",
     "U – " + c("userspace/libc/src/stdlib.c") + " (the stub), hook in " + c("exit()") +
     " in " + c("exec.c") + ". No syscall. (A0-13 – the real A0 task of 14 Oct 2026)", "triage U"),
    ("Task: \"malloc/free should work\" – type, first file?",
     "U – " + c("stdlib.c") + ": a static pool + block headers (A1-14).", "triage U"),
    ("Task: \"add strncpy to the libc\" – type, first step?",
     "U – declare it in " + c("userspace/libc/include/string.h") + " (missing!), implement in "
     + c("string.c") + " (A0-14).", "triage U"),
    ("Task: \"pthread_mutex_lock must really lock\" – type?",
     "U – " + c("pthread.c") + ": " + c("while (__sync_lock_test_and_set(lock, 1)) sched_yield();")
     + " (A1-15).", "triage U"),
    ("Task: \"sleep(seconds) should work\" – type?",
     "S + E – user space has no clock: a syscall that yields until "
     + c("Scheduler::instance()->getTicks()") + " reached the target (A1-18).", "triage S E"),
    ("Task: \"waitpid returns the child's exit code\" – type, where does the code live?",
     "S – the exit code must outlive the process: a table in the " + c("ProcessRegistry") +
     ", filled from " + c("~UserProcess") + " (A1-19).", "triage S"),
    ("Task: \"heap pages are mapped on first access\" – type, where?",
     "S + E – limit in " + c("UserProcess") + ", a new case <b>first</b> in the valid branch of "
     + c("PageFaultHandler::handlePageFault") + " (A0-03).", "triage E"),
    ("Task: \"code pages must be read-only\" – type, where?",
     "E (loader) – " + c("Loader::loadPage") + ": " + c("mapPage(…, writeable = false)") +
     " for segments without PF_W (A0-05).", "triage E"),
    ("Task: \"when F3 is pressed, do X with the process\" – type, where?",
     "E (key) – " + c("Console::handleKey") + " only sets a flag; the top of "
     + c("Syscall::syscallException") + " does the work in the process' own context (ex07, A2-06).",
     "triage E"),
    ("Task: \"a flagged thread runs twice as long\" – where?",
     "E – " + c("Scheduler::schedule()") + " keeps " + c("currentThread") +
     " for one more slice (A1-04).", "triage E"),
    ("Task: \"count page faults per process\" – type, where?",
     "A + E – " + c("pageFaultHandler") + " in " + c("arch/x86/64/source/InterruptUtils.cpp") +
     "; interrupts are off there → atomics, no lock (A1-11).", "triage A"),
    ("Task: \"call a user function when the process segfaults\" – type?",
     "K + E – the page fault handler sets " + c("user_registers_->rip / rsp / rdi") +
     " so the thread continues in the handler (A0-06).", "triage K"),
    ("Task: \"a syscall with 15 arguments\" – type, how?",
     "A – 4 values in registers, the other 11 in an array on the user stack, its address as the "
     "5th argument; the kernel copies it in (A1-13).", "triage A"),
    ("Task: \"a kernel thread that does X every few ticks\" – where?",
     "E – a class derived from " + c("Thread") + " with " + c("Run()") + "; "
     + c("Scheduler::instance()->addNewThread(new X)") + " in " + c("startup()") +
     " (main.cpp) (ex06, A2-10).", "triage E"),
    ("Task: \"never execute data or stack\" – where?",
     "E – " + c("execution_disabled") + " bit: in " + c("Loader::loadPage") + " (no PF_X) and "
     + c("UserProcess::mapStackPage") + " (A2-08).", "triage E"),
    ("Task: \"mmap(MAP_ANONYMOUS)\" – type, where?",
     "S + E – regions in " + c("UserProcess") + ", the syscall only reserves, the page fault "
     "handler maps on first access (A2-13).", "triage S E"),
    ("Task: \"store something on the disk that survives a reboot\" – where?",
     "E (boot) + S – " + c('BDManager::getInstance()->getDeviceByName("idea2")') + ", "
     + c("readData/writeData") + "; read it at boot (A2-11).", "triage E"),
]

# --------------------------------------------------------------------------------------------
DECKS["2 Where is what"] = [
    ("Where does every user program start?",
     c("_start") + " in " + c("userspace/libc/src/nonstd.c") + ": " + c("exit(main());"), "where libc"),
    ("Where is exit() for user programs – and where would atexit handlers run?",
     c("userspace/libc/src/exec.c") + ": " + c("exit()") + " – before " + c("__syscall(sc_exit, …)")
     + ". " + c("_exit()") + " must not run them.", "where libc"),
    ("Where are syscall numbers defined?",
     c("common/include/kernel/syscall-definitions.h") + " – included by kernel <b>and</b> libc.",
     "where syscall"),
    ("Where is the big syscall switch?",
     c("Syscall::syscallException") + " in " + c("common/source/kernel/Syscall.cpp"), "where syscall"),
    ("The six places of a new syscall?",
     "1 " + c("syscall-definitions.h") + " · 2 " + c("Syscall.h") + " · 3 " + c("Syscall.cpp")
     + " case (+ break) · 4 " + c("Syscall.cpp") + " implementation · 5 " + c("nonstd.h") +
     " · 6 " + c("nonstd.c") + " (" + c("__syscall") + ") – then a test in "
     + c("userspace/tests/"), "where syscall"),
    ("Which registers carry a syscall?",
     "number in " + c("rax") + ", arguments in " + c("rbx rcx rdx rsi rdi") + ", result back in "
     + c("rax") + " (" + c("arch/x86/64/userspace/syscalls.c") + ", " + c("int $0x80") + ").",
     "where syscall registers"),
    ("Path of a syscall from int $0x80 to your function?",
     c("arch_syscallHandler") + " (asm) → " + c("syscallHandler") + " (InterruptUtils.cpp, "
     "interrupts on) → " + c("Syscall::syscallException(rax, rbx, …)") + " → your case → return "
     "value into " + c("user_registers_->rax"), "where syscall"),
    ("Path of a page fault?",
     c("arch_pageFaultHandler") + " → " + c("pageFaultHandler") + " (InterruptUtils.cpp) → "
     + c("PageFaultHandler::enterPageFault") + " → " + c("handlePageFault") + " → "
     + c("checkPageFaultIsValid") + " → stack growth / guard page / " + c("loadPage"), "where pf"),
    ("What does checkPageFaultIsValid reject?",
     "null page (&lt; 4096), kernel addresses, and faults on <b>present</b> pages (protection: "
     "write to read-only, NX) → exit 9999.", "where pf"),
    ("Where are a thread's stack pages added on demand?",
     c("handlePageFault") + ": " + c("isUserStackFault") + " → " + c("allocateStackPage") + " + "
     + c("mapStackPage") + " down to the fault (under " + c("thread_lock_") + ").", "where pf stack"),
    ("Where is a page loaded from the ELF binary? What if no segment covers it?",
     c("Loader::loadPage(address, arch_memory)") + " (common/source/kernel/Loader.cpp) – "
     "no segment → " + c("Syscall::exit(666)"), "where loader"),
    ("Where are the F-keys handled? Which are free?",
     c("Console::handleKey") + " (common/source/console/Console.cpp). F9–F12 are debug output, "
     "Shift+Fn switches terminals – F2–F8 are free.", "where key"),
    ("Where is the next thread chosen?",
     c("Scheduler::schedule()") + " – round robin over " + c("threads_") + ", the first "
     + c("schedulable()") + " one, then rotate.", "where scheduler"),
    ("What runs on every timer tick?",
     c("irqHandler_0") + " (InterruptUtils.cpp) → " + c("Scheduler::incTicks()") + " → "
     + c("Scheduler::schedule()") + " → " + c("arch_contextSwitch"), "where timer"),
    ("Where is the kernel booted, where do you create kernel-wide objects?",
     c("startup()") + " in " + c("common/source/kernel/main.cpp"), "where boot"),
    ("Who starts the shell?",
     "the " + c("ProcessRegistry") + " kernel thread: " + c("Run()") + " mounts /usr and calls "
     + c("createProcess") + " for " + c("user_progs.h"), "where boot"),
    ("Where are the PTE bits (present, writeable, NX, free bits)?",
     c("PageTableEntry") + " in " + c("arch/x86/64/include/paging-definitions.h") + " – "
     + c("ignored_1/2/3") + " are free for you.", "where memory"),
    ("Where is USER_BREAK and what is it?",
     c("arch/x86/64/include/offsets.h") + ": " + c("0x800000000000") + " – everything below is user "
     "space.", "where memory"),
    ("Where do you add your own debug flag?",
     c("common/include/console/debug.h") + ": " + c("const size_t NAME = Ansi_Cyan | OUTPUT_ENABLED;")
     + " then " + c('debug(NAME, "…")'), "where debug"),
    ("Where is the safe user-string copy helper?",
     c("Syscall::copyUserString(user_ptr, buffer, size)") + " – checks every byte against "
     + c("USER_BREAK") + ".", "where syscall"),
    ("Where is a new user test program put? Does CMake need a change?",
     c("userspace/tests/name.c") + " → " + c("/usr/name.sweb") + "; no – every .c there is built.",
     "where test"),
    ("Where are the swap partition and the other disks?",
     c('BDManager::getInstance()->getDeviceByName("idea2")') + " → " + c("readData/writeData(offset, size, buf)")
     + " (idea1 = /usr, idea2 = unused swap partition).", "where disk"),
    ("Which libc functions are only stubs in your team's code?",
     c("malloc/free/calloc/realloc") + ", " + c("atexit") + ", " + c("sem_*") + ", "
     + c("pthread_mutex_* / cond_* / spin_*") + ", " + c("pthread_detach") + ", " + c("sleep") + ", "
     + c("clock") + ", " + c("brk/sbrk") + ", " + c("waitpid") + ", " + c("mmap/munmap/shm_*/mprotect")
     + " – and " + c("strcpy/strncpy/strcat") + " are missing completely (ATLAS.md §5).", "where libc"),
]

# --------------------------------------------------------------------------------------------
DECKS["3 Team API"] = [
    ("The calling process (team code)?", c("currentThread->getProcess()") + " – a "
     + c("UserProcess*") + ". Never " + c("(UserProcess*) currentThread") + " (compiles, reads garbage).",
     "api"),
    ("The page tables of the calling process?", c("currentThread->getProcess()->getAddressSpace()")
     + " – an " + c("ArchMemory&") + ", owned by the UserProcess.", "api"),
    ("The page-table lock in your team's code?", c("process->getThreadLock()") + " – the Mutex "
     + c("thread_lock_") + "; also protects " + c("threads_") + " and the stack slots.", "api lock"),
    ("Does your team's loadPage hold the page-table lock?", "<b>No</b> – the page fault handler "
     "releases " + c("thread_lock_") + " before " + c("loadPage") + ". Read-only/zero pages: map "
     "them with " + c("mapPage(…, writeable)") + " in one step.", "api lock loader"),
    ("Per-process state goes where? Per-thread state?", "per process → member of "
     + c("UserProcess") + "; per user thread → member of " + c("UserThread") + " ("
     + c("(UserThread*) currentThread") + ").", "api"),
    ("ArchMemory::mapPage – signature and return?", c("[[nodiscard]] bool mapPage(vpn, ppn, user_access)")
     + " – false if the vpn is already mapped (then free your ppn!).", "api memory"),
    ("What does unmapPage do – and not do?", "removes the entry, <b>frees the physical page</b> and "
     "empty page tables – does <b>not</b> flush the TLB (" + c("ArchMemory::flushTlb()") + ").",
     "api memory"),
    ("How do you find out whether a vpn is mapped?", c("getAddressSpace().resolveMapping(vpn)")
     + " → " + c(".page_size") + " (0 = not mapped), " + c(".page_ppn") + ", " + c(".pt[.pti]")
     + " = the PTE.", "api memory"),
    ("How does the kernel read/write a physical page?", c("(char*) ArchMemory::getIdentAddressOfPPN(ppn)")
     + " – the ident mapping, works without any user mapping.", "api memory"),
    ("Get / give back a physical page?", c("PageManager::instance()->allocPPN()") + " (zeroed) / "
     + c("freePPN(ppn)") + " – never after " + c("unmapPage") + " (double free).", "api memory"),
    ("Find a thread of your process by id?", c("process->findThreadDataByTid(tid)") + " under "
     + c("thread_lock_") + " → " + c("UserThreadData") + "; " + c("->thread_") + " is nullptr once it exited.",
     "api threads"),
    ("Fields of UserThreadData?", c("thread_") + ", " + c("exit_value_") + ", " + c("tid_") + ", "
     + c("exited_") + ", " + c("joined_") + ", " + c("requested_cancel_"), "api threads"),
    ("How does pthread_join wait in your team's code?", c("while (!thread_data->exited_) thread_exited_.wait();")
     + " – " + c("exitThread") + " broadcasts " + c("thread_exited_") + ".", "api threads"),
    ("What does exitProcess do?", "kills every other thread of the process (" + c("kill()")
     + ") and deletes their UserThreadData – POSIX: exit ends the whole process.", "api threads"),
    ("Where does a new thread start executing?", "in libc's " + c("pthread_start(start_routine, arg)")
     + " (rip), " + c("rdi = start_routine") + ", " + c("rsi = arg") + " (" + c("setThreadArguments")
     + "), then " + c("pthread_exit(start_routine(arg))"), "api threads registers"),
    ("Stack layout of your team's threads?", "a slot per thread: " + c("USERSTACK_PAGES") + " = 4 pages + 1 "
     "guard page, counted down from USER_BREAK; one page mapped at start, more on page faults.",
     "api stack"),
    ("When is a UserProcess deleted?", "when its last thread is gone: CleanupThread → "
     + c("~UserThread") + " → " + c("threadExited()") + " → " + c("delete this") + " → "
     + c("~UserProcess") + " (deletes loader + address space).", "api process"),
    ("Saved user registers of the current thread?", c("currentThread->user_registers_") + " → "
     + c("rip, rsp, rax, rbx, rcx, rdx, rsi, rdi, r8–r15, rflags, cr3") + " (ArchThreads.h)", "api registers"),
    ("Timer ticks since boot? How fast?", c("Scheduler::instance()->getTicks()") + " – 18.2 per second "
     "(1193180 / 65536), one tick ≈ 55 ms.", "api timer"),
    ("Syscall numbers your team uses?", "1000–1002 pthread_create/exit/self · 1500 mkdir · 1501–1503 "
     "join/cancel/testcancel · 1998/1999 counts · 2000–2002 fork/getpid/getppid · 2500 execv", "api syscall"),
]

# --------------------------------------------------------------------------------------------
DECKS["4 Locking"] = [
    ("Check-then-act rule?", "the check and the change must be <b>one</b> critical section – "
     "otherwise another thread changes the state in between (e.g. two threads find the same free slot).",
     "lock"),
    ("Why allocPPN before taking the lock?", "keeps the critical section short; give the page back "
     "with " + c("freePPN") + " if you did not use it.", "lock"),
    ("Never call Syscall::exit while …?", "holding a lock – the thread dies, the lock is never released.",
     "lock"),
    ("Why write user memory only after releasing the lock?", "writing user memory can page-fault; "
     "the page fault handler takes " + c("thread_lock_") + " → deadlock if you hold it.", "lock"),
    ("Mutex or SpinLock for your code?", "Mutex (sleeps) – SpinLock only for very short sections, "
     "never allocate (" + c("new") + ") while holding one.", "lock"),
    ("Locks with interrupts off (interrupt handler, schedule())?", "impossible – set an atomic flag, "
     "do the work later in a thread (ex07: F-key flag handled at syscall entry).", "lock"),
    ("Why broadcast, not signal, on thread_exited_?", "several waiters wait for <b>different</b> things "
     "(join of thread 3, join of thread 5) – signal could wake the wrong one.", "lock condition"),
    ("Why always wait in a while loop on a Condition?", "after waking up the state may have changed "
     "again (another thread was faster) – and wake-ups may be spurious.", "lock condition"),
    ("ScopeLock?", c("ScopeLock lock(mutex);") + " – acquired in the constructor, released at the end "
     "of the scope (every return).", "lock"),
    ("Two locks – what rule?", "always the same order everywhere (e.g. " + c("thread_lock_")
     + " → PageManager), or never hold both.", "lock"),
    ("Removing rights (unmap, read-only, swap-out) – what else?", c("ArchMemory::flushTlb()")
     + " – inside the critical section, before anyone relies on the change.", "lock tlb"),
    ("Why does a user-space lock need an atomic instruction?", "between reading 0 and writing 1 the "
     "timer can switch threads – both see 0 and both enter. " + c("__sync_lock_test_and_set")
     + " (xchg) does read+write in one step.", "lock libc"),
    ("Why may the atexit counter be plain, but a lock not?", "registering from two threads at the "
     "same moment is rare (mention it); a lock is used by many threads at once by definition.", "lock libc"),
]

# --------------------------------------------------------------------------------------------
DECKS["5 Traps"] = [
    ("case in the syscall switch – most common bug?", "missing " + c("break;") + " → falls through "
     "into the next syscall.", "trap"),
    ("Returning -1 from a size_t syscall?", c("return (size_t) -1;") + " – not " + c("-1U")
     + " (that is 0xFFFFFFFF).", "trap"),
    ("NULL from a size_t function?", c("return 0;"), "trap"),
    ("Next page in a loop over vpns?", c("vpn + 1") + " – not " + c("vpn + PAGE_SIZE") + ".", "trap"),
    ("address ↔ vpn?", c("vpn = address / PAGE_SIZE") + ", " + c("address = vpn * PAGE_SIZE")
     + ", offset = " + c("address % PAGE_SIZE"), "trap memory"),
    ("Overflow-safe user pointer check?", c("ptr != 0 && size <= USER_BREAK && ptr <= USER_BREAK - size")
     + " – not " + c("ptr + size <= USER_BREAK") + " (can overflow).", "trap"),
    ("page_size – field or function?", "field: " + c("m.page_size") + " (no parentheses).", "trap"),
    ("User pointer: check first or write first?", "<b>check first</b> – writing to NULL in the kernel "
     "page-faults and kills the process (exit 9999).", "trap"),
    ("New method in a header – compiler says 'private within this context'?", "it was added after "
     + c("private:") + " – move it into the " + c("public:") + " part.", "trap"),
    ("Declaration in nonstd.h and definition in nonstd.c differ?", "'conflicting types' – return "
     "type and parameters must be identical.", "trap"),
    ("Test: 'makes pointer from integer without a cast'?", "the libc wrapper returns " + c("size_t")
     + " where the test expects a pointer – return " + c("void*") + " and cast: "
     + c("(void*) __syscall(…)"), "trap"),
    ("New Mutex member: compile error -Werror=reorder?", "the initializer list must follow the "
     "declaration order – or use a default member initializer " + c('Mutex m_{"name"};'), "trap"),
    ("Exit codes 666 vs 9999?", "666: " + c("loadPage") + " found no segment for the address · 9999: "
     "invalid page fault (null pointer, kernel address, protection).", "trap pf"),
    ("strncpy with a source of n or more characters?", "writes exactly n bytes and <b>no</b> "
     "terminating NUL.", "trap libc"),
    ("Same page fault 10 times in a row?", "your handler returned without fixing the cause – "
     + c("InterruptUtils::countPageFault") + " halts after 10.", "trap pf"),
    ("Kernel heap allocation that lives until shutdown (e.g. a growing list)?", "SWEB's leak check "
     "reports it at " + c("exit") + " – use a fixed array, or free it when the owner ends.", "trap"),
]

# --------------------------------------------------------------------------------------------
DECKS["6 Tasks"] = [
    ("A0-01 meminfo – key steps?", "check the user pointer (struct crosses USER_BREAK?), "
     + c("getTotalNumPages/getNumFreePages") + ", " + c("countUserPages()") + " (a copy of "
     + c("~ArchMemory") + "'s walk, count present PTEs) under " + c("getThreadLock()") + ", write after unlock.",
     "task a0"),
    ("A0-02 allocpage – the race?", "search + map must be one critical section, else two threads "
     "find the same free slot. freepage: aligned? in region? mapped? → " + c("unmapPage") + " + "
     + c("flushTlb") + ", no " + c("freePPN") + ".", "task a0"),
    ("A0-03 heaplimit – where is the heap mapped?", "in the page fault handler, <b>first</b> in the "
     "valid branch: " + c("mapHeapPage(vpn)") + " checks the limit and maps under " + c("thread_lock_")
     + "; shrinking unmaps + flushTlb.", "task a0"),
    ("A0-12 alias page – why a kernel panic at exit?", "one frame mapped twice is freed twice by "
     + c("~ArchMemory") + " ('Double free PPN') – unmap the alias with " + c("unmapPage(vpn, false)")
     + " in " + c("~UserProcess") + " first.", "task a0"),
    ("A0-13 atexit – order and nested exit?", "reverse order of registration; pop each handler "
     "<b>before</b> calling it, so a handler that calls " + c("exit()") + " continues with the rest; "
     + c("_exit") + " runs none.", "task a0 U"),
    ("A0-07 upcall – how does the kernel call a user function?", "push the old " + c("rip")
     + " onto the user stack (via " + c("checkAddressValid") + "), " + c("rip = function")
     + ", " + c("rdi = argument") + " – the function's " + c("ret") + " returns behind the syscall.",
     "task a0 K"),
    ("A1-02 pthread_multiple – all or nothing?", "create all thread objects first (not scheduled), "
     "on failure delete them (" + c("discardThread") + "), only then " + c("addNewThread") + " for all.",
     "task a1"),
    ("A1-03 barrier – why a generation counter?", "a woken thread checks 'did my round open?', not "
     "'waiting == 0' – the next round may already be filling up.", "task a1"),
    ("A1-14 malloc – how does free know the size?", "a header right before the block: "
     + c("(struct BlockHeader*) ptr - 1"), "task a1 U"),
    ("A1-17 condition variable – the lost wake-up?", "read the generation <b>before</b> releasing the "
     "mutex; a signal between unlock and check then still changes the number.", "task a1 U"),
    ("A1-19 waitpid – where does the exit code live?", "in the " + c("ProcessRegistry")
     + " (fixed table) – " + c("~UserProcess") + " runs long after exit; " + c("Syscall::exit")
     + " stores " + c("exit_code_") + " first.", "task a1"),
    ("A1-20 pthread_detach – what is freed when?", "the UserThreadData: at once if the thread "
     "already ended, else in " + c("exitThread") + "; join of a detached thread → -1.", "task a1"),
    ("A2-01 swap – how does a PTE say 'swapped out'?", c("present = 0") + ", " + c("ignored_2 = PTE_SWAPPED")
     + ", " + c("page_ppn = slot") + " – the CPU ignores non-present entries.", "task a2"),
    ("A2-01 swap-out order?", "1. present = 0 + flushTlb (nobody writes meanwhile) · 2. write to disk "
     "· 3. mark swapped + free the frame – all under the page-table lock.", "task a2"),
    ("A2-09 copy-on-write of the zero page?", "map the shared zero page read-only; a write faults "
     "(present + writing) → new frame, writeable, flushTlb.", "task a2"),
]

# --------------------------------------------------------------------------------------------
DECKS["7 POSIX"] = [
    ("exit vs _exit?", c("exit") + " runs the atexit handlers (and flushes stdio), " + c("_exit")
     + " ends at once.", "posix"),
    ("atexit – limits and return?", "at least 32 handlers, 0 on success, non-zero (−1 here) on error.",
     "posix"),
    ("pthread_join on yourself / a joined thread?", "error (−1 in SWEB) – EDEADLK / EINVAL in POSIX.",
     "posix"),
    ("Is pthread_join a cancellation point?", "yes – a cancelled thread sleeping in join must end "
     "(your team's code: not yet, see A1-06).", "posix"),
    ("Deferred cancellation?", "the target only ends at a cancellation point (syscalls, "
     + c("pthread_testcancel") + ", join) – never in the middle of a critical section.", "posix"),
    ("What does pthread_detach mean?", "nobody will join the thread: its resources are freed when it "
     "ends; joining it is an error.", "posix"),
    ("waitpid(pid, &status, 0)?", "waits for child pid, returns its pid, status = how it ended; "
     "−1 if pid is not a child.", "posix"),
    ("A zombie?", "a process that has ended, but its parent has not waited for it yet – only its "
     "exit status is kept.", "posix"),
    ("strcpy vs strncpy?", c("strcpy") + " copies up to and including the NUL; " + c("strncpy")
     + " exactly n bytes – pads with 0, or leaves the result unterminated.", "posix"),
    ("pthread_cond_wait(cond, mutex)?", "atomically: release mutex + wait; on return it holds the "
     "mutex again. Call it in a while loop.", "posix"),
    ("sem_wait / sem_post?", c("sem_wait") + ": wait until value &gt; 0, then decrement; "
     + c("sem_post") + ": increment (wakes a waiter).", "posix"),
    ("mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)?", "len bytes of new, "
     "zeroed memory; failure: MAP_FAILED = (void*) −1.", "posix"),
]

# --------------------------------------------------------------------------------------------
DECKS["8 Basics"] = [
    ("Page vs frame?", "page = virtual (vpn), frame = physical (ppn); both 4096 bytes. The page "
     "table maps vpn → ppn.", "basics"),
    ("x86-64 page-table tree?", "PML4 → PDPT → PD → PT, 512 entries each, 9 bits of the address per "
     "level; the lower half of the PML4 = user space.", "basics"),
    ("TLB?", "the CPU's cache of translations – stale after a change that removes rights → flush.",
     "basics"),
    ("Lazy loading?", "a page of the program is loaded only on its first access (page fault → "
     + c("loadPage") + ").", "basics"),
    ("ELF: sections vs segments?", "sections (.text .rodata .data .bss) are the linker's view; LOAD "
     "segments are what the loader maps. .bss = MemSiz &gt; FileSiz (zeroed).", "basics"),
    ("Red zone?", "128 bytes below rsp that a function may use without moving rsp – skip it before "
     "pushing something onto a user stack from the kernel.", "basics registers"),
    ("Why must the kernel check user pointers?", "a user program can pass any address – also a "
     "kernel address or one that crosses USER_BREAK; the kernel would read/write it with full rights.",
     "basics"),
    ("Timer frequency in SWEB?", "18.2 Hz – a time slice is about 55 ms.", "basics"),
]


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    out = os.path.join(here, "ag-interviews.txt")
    fronts = set()
    lines = ["#separator:tab", "#html:true", "#notetype:Basic", "#deck column:3", "#tags column:4"]
    count = 0
    for deck, cards in DECKS.items():
        for front, back, tags in cards:
            assert front not in fronts, "duplicate front: " + front
            fronts.add(front)
            for field in (front, back):
                assert "\t" not in field and "\n" not in field
            row = [front, back.replace("\n", "<br>"), ROOT + "::" + deck,
                   " ".join("ag::" + t for t in tags.split())]
            lines.append("\t".join(row))
            count += 1
    with open(out, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"{count} cards -> {out}")


if __name__ == "__main__":
    main()
