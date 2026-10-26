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
     "U – " + c("userspace/libc/src/stdlib.c") + ", hook in " + c("exit()") +
     " in " + c("exec.c") + ". No syscall. (A0-13 – the real A0 task of 14 Oct 2026; your version is "
     "in the team's main since 18 Oct: " + c("__runAtexitFunctions()") + ")", "triage U"),
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
    ("Where is exit() for user programs – and where do the atexit handlers run?",
     c("userspace/libc/src/exec.c") + ": " + c("exit()") + " calls " + c("__runAtexitFunctions()")
     + " (stdlib.c) before " + c("__syscall(sc_exit, …)") + ". " + c("_exit()") + " does not.", "where libc"),
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
     c("malloc/free/calloc/realloc") + ", " + c("sem_*") + ", "
     + c("pthread_mutex_* / cond_* / spin_*") + ", " + c("pthread_setcancelstate/type") + ", " + c("sleep") + ", "
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
     + c("exited_") + ", " + c("joined_") + ", " + c("requested_cancel_") + ", " + c("detached_"), "api threads"),
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
    ("Syscall numbers your team uses?", "1000–1003 pthread_create/exit/self/equal · 1500 mkdir · 1501–1504 "
     "join/cancel/testcancel/detach · 1998/1999 counts · 2000–2002 fork/getpid/getppid · 2500 execv", "api syscall"),
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
     "already ended, else in " + c("exitThread") + "; join of a detached thread → -1 (your "
     + c("detachThread") + ", syscall 1504).", "task a1"),
    ("A1-21 forkall – what makes a thread, and what does the child copy?", "a thread = its saved registers "
     "(" + c("user_registers_") + ") + its stack (in the address space). Copy every " + c("UserThreadData")
     + " (same tid) and every live thread (" + c("UserThread(child, original)") + " + stack top), under "
     + c("thread_lock_") + "; a thread inside a syscall gets rax = -1.", "task a1"),
    ("A1-23 pthread_switch(a, b) – what changes, under which lock?", "the ids swap in <b>two</b> places: the records ("
     + c("UserThreadData::tid_") + ", searched by join/cancel/detach) and the thread objects (" + c("Thread::setTID")
     + ", read by pthread_self) – all under " + c("thread_lock_") + "; refused for a == b, unknown ids, or while joined.", "task a1"),
    ("A1-22 context switch – which registers, and where is rip?", "only the callee-saved " + c("rbx rbp r12–r15")
     + " (+ " + c("rsp") + "): push them, save rsp, load the other rsp, pop, " + c("ret")
     + ". rip is the return address on the stack.", "task a1"),
    ("Bug in your team's fork (19 Oct)?", "the child's " + c("UserThread") + " copy does not get "
     + c("user_stack_top_") + " / " + c("lowest_mapped_stack_vpn_") + " → a child that needs a new stack "
     "page is killed with 666.", "api threads"),
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
    ("Is pthread_join a cancellation point?", "yes – a cancelled thread sleeping in join must end. "
     "Your team's code (since 18 Oct): " + c("cancelThread") + " broadcasts, the wait loop returns "
     + c("JOIN_CANCELED") + ", " + c("Syscall::pthreadJoin") + " calls " + c("exitThread") + " outside the lock.", "posix"),
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

# --------------------------------------------------------------------------------------------
# Every file of your team's code you could be sent to (x86-64; arch/x86/32 and arch/arm are not
# built). (path, what is in it, reverse question or None)
FILES = [
    # ---- kernel core: common/source/kernel + common/include/kernel
    ("common/source/kernel/Syscall.cpp", "the syscall switch " + c("syscallException") + " and every syscall: exit, write, read, open, close, createprocess, pthreadCreate/Exit/Join/Cancel/Self, fork, execv, getpid, mkdir, getThreadCount; helper " + c("copyUserString"), "Which file has every syscall implementation?"),
    ("common/include/kernel/Syscall.h", "the declarations " + c("static size_t x(…)") + " of all syscalls", None),
    ("common/include/kernel/syscall-definitions.h", "the " + c("sc_…") + " numbers – shared by kernel and libc", None),
    ("common/source/kernel/UserProcess.cpp", "the process: " + c("create, startProcess, fork, exec, createThread, registerThread, threadExited") + ", stacks (" + c("allocateNewStack, mapStackPage, isGuardPage") + "), " + c("exitThread, joinThread, cancelThread, exitProcess") + ", " + c("getAddressSpace, getLoader, getThreadLock") + ", destructor", "Which file creates processes and threads and ends them?"),
    ("common/include/kernel/UserProcess.h", c("UserThreadData") + " struct, members " + c("arch_memory_, loader_, thread_lock_, thread_exited_, threads_") + ", " + c("USERSTACK_PAGES, GUARD_PAGE") + " – per-process state goes here", "Where do you add per-process state?"),
    ("common/source/kernel/UserThread.cpp", "a user thread: the two constructors (new thread: user registers, stack top, address space; forked thread: copy of the parent's registers, rax = 0), " + c("Run, resetForExec") + ", stack vpns", None),
    ("common/include/kernel/UserThread.h", c("class UserThread : public Thread") + " – " + c("user_stack_top_, lowest_mapped_stack_vpn_") + "; per-thread state goes here", "Where do you add per-thread state of user threads?"),
    ("common/source/kernel/Thread.cpp", "base class of every thread: kernel stack (+ canary), " + c("kill()") + ", state, name, TID, terminal, " + c("printBacktrace") + ", " + c("threadStartHack") + " (start of kernel threads)", None),
    ("common/include/kernel/Thread.h", c("kernel_registers_, user_registers_, switch_to_userspace_, getProcess(), state_") + "; " + c("ThreadState") + " Running/Sleeping/ToBeDestroyed", None),
    ("common/source/kernel/Scheduler.cpp", c("schedule") + " (round robin), " + c("addNewThread, yield, sleep, wake, cleanupDeadThreads, printThreadList (F12), incTicks/getTicks, getNextTid, lockScheduling"), "Which file decides which thread runs next?"),
    ("common/source/kernel/Loader.cpp", "the ELF binary of a process: " + c("readHeaders, prepareHeaders, getEntryFunction, loadPage") + " (lazy loading, exit 666), debug info for backtraces", "Which file loads a program's pages from its file?"),
    ("common/source/kernel/ProcessRegistry.cpp", "kernel thread: mounts /usr, starts the programs of " + c("user_progs.h") + " (the shell); " + c("createProcess, registerPID") + ", counts processes (" + c("processStart/Exit/Count") + "), waits for all at shutdown", "Which kernel thread starts the shell and counts processes?"),
    ("common/include/kernel/user_progs.h", "the list of programs started at boot – only " + c("/usr/shell.sweb") + " (don't rename the variable: automated tests)", None),
    ("common/source/kernel/Mutex.cpp", "the sleeping lock: " + c("acquire, release, acquireNonBlocking, isFree") + " – waiters sleep in the scheduler", None),
    ("common/source/kernel/SpinLock.cpp", "the spinning lock (" + c("ArchThreads::testSetLock") + " + yield) – very short sections, no new/delete inside", None),
    ("common/source/kernel/Condition.cpp", c("wait, waitAndRelease, signal, broadcast") + " – always used with its Mutex", None),
    ("common/source/kernel/Lock.cpp", "base class of Mutex/SpinLock/Condition: waiters list, holding list, deadlock and misuse checks (asserts with a backtrace)", "Which file detects deadlocks and wrong lock releases?"),
    ("common/include/kernel/ScopeLock.h", c("ScopeLock lock(mutex);") + " – acquire in the constructor, release in the destructor", None),
    ("common/source/kernel/CleanupThread.cpp", "kernel thread that deletes dead threads: " + c("Scheduler::cleanupDeadThreads") + " → " + c("delete thread"), "Which thread deletes threads that called kill()?"),
    ("common/source/kernel/IdleThread.cpp", "runs when nothing else can: " + c("ArchCommon::idle()") + " (hlt until the next interrupt)", None),
    ("common/source/kernel/main.cpp", c("startup()") + ": boot order – page manager, console, scheduler, interrupts, timer, file systems, block devices, keyboard, kernel threads (Console, ProcessRegistry); create kernel-wide objects here", None),
    # ---- memory: common/source/mm + arch/x86/64
    ("common/source/mm/PageFaultHandler.cpp", c("checkPageFaultIsValid, isUserStackFault, handlePageFault, enterPageFault") + " – stack growth, guard page, " + c("loadPage") + "; new cases (heap, swap-in, COW) go first in the valid branch", None),
    ("common/source/mm/PageManager.cpp", "physical pages: bitmap of all frames, " + c("allocPPN") + " (zeroed), " + c("freePPN") + ", counts, " + c("printBitmap") + " (F9)", "Which file hands out physical pages?"),
    ("common/source/mm/KernelMemoryManager.cpp", "the kernel heap behind " + c("new/delete/kmalloc") + ": segments, " + c("ksbrk") + ", leak tracing (" + c("startTracing") + ")", "Which file is the kernel heap (new/delete)?"),
    ("common/source/mm/new.cpp + kmalloc.cpp", c("operator new/delete") + " and " + c("kmalloc/kfree/krealloc") + " → KernelMemoryManager", None),
    ("arch/x86/64/source/ArchMemory.cpp", "page tables: " + c("mapPage, unmapPage, resolveMapping, checkAddressValid, flushTlb, getIdentAddressOfPPN") + ", kernel mappings, copy constructor (fork), destructor (frees all user pages)", "Which file changes page tables?"),
    ("arch/x86/64/include/ArchMemory.h", c("ArchMemoryMapping") + " (" + c("page_size, page_ppn, pt, pti, …") + "), the class – new methods in " + c("public:"), None),
    ("arch/x86/64/include/paging-definitions.h", c("PAGE_SIZE") + ", the entry bit fields (" + c("present, writeable, user_access, accessed, dirty, ignored_1/2/3, execution_disabled, page_ppn") + "), entries per table", "Where are the bits of a page-table entry defined?"),
    ("arch/x86/64/include/offsets.h", c("USER_BREAK") + " (0x800000000000), " + c("KERNEL_START") + ", " + c("IDENT_MAPPING_START"), None),
    ("arch/x86/64/source/init_boottime_pagetables.cpp", c("initialisePaging") + " (the page tables used while booting), " + c("removeBootTimeIdentMapping"), None),
    ("arch/x86/64/utils/kernel-ld-script.ld", "the linker script of the kernel: entry point, where the sections go (virtual + physical address), alignment", "Where is defined where the kernel's sections are placed in memory?"),
    # ---- interrupts, threads, CPU: arch/x86/64 + arch/x86/common + arch/common
    ("arch/x86/64/source/InterruptUtils.cpp", "the C side of every interrupt: " + c("syscallHandler, pageFaultHandler, irqHandler_0") + " (timer), " + c("irqHandler_1") + " (keyboard), IDE IRQs 14/15, " + c("errorHandler") + "; IDT setup; " + c("countPageFault"), "Which file contains the C entry of every interrupt?"),
    ("arch/x86/64/source/arch_interrupts.S", "assembly stubs: save all registers into " + c("currentThreadRegisters") + ", call the C handler, " + c("arch_contextSwitch") + " (load registers, iretq)", "Where are registers saved when an interrupt arrives?"),
    ("arch/x86/64/source/ArchThreads.cpp", c("createUserRegisters, createKernelRegisters, setThreadArguments") + " (rdi/rsi), " + c("changeInstructionPointer, setAddressSpace") + " (cr3), " + c("yield") + ", atomics " + c("testSetLock, atomic_add, atomic_set") + ", " + c("printThreadRegisters"), "Which file creates a thread's registers?"),
    ("arch/x86/64/include/ArchThreads.h", c("struct ArchThreadRegisters") + ": rip, cs, rflags, rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8–r15, segments, rsp0, cr3, fpu", "Where is the struct with a thread's saved registers?"),
    ("arch/x86/64/source/ArchInterrupts.cpp", "enable/disable interrupts, " + c("testIFSet") + ", timer on/off + " + c("setTimerFrequency") + ", keyboard on/off, " + c("EndOfInterrupt"), None),
    ("arch/x86/64/source/ArchCommon.cpp", "multiboot info: usable memory regions, modules, VESA/text console creation; " + c("idle") + " (hlt), " + c("drawStat") + " (status line), " + c("drawHeartBeat"), None),
    ("arch/x86/64/source/arch_backtrace.cpp", c("backtrace") + " / " + c("backtrace_user") + ": walks the frame pointers of a thread", None),
    ("arch/x86/64/userspace/syscalls.c", c("__syscall") + ": " + c("int $0x80") + ", number in rax, args in rbx rcx rdx rsi rdi, result from rax", "Which file contains the instruction that enters the kernel from user space?"),
    ("arch/x86/64/include/types.h + ElfFormat.h", "the kernel's integer types (" + c("uint64, size_t, pointer") + "); the ELF64 structures (" + c("Elf::Ehdr, Elf::Phdr") + ") for the loader", None),
    ("arch/x86/common/source/8259.cpp", "the interrupt controller (PIC): " + c("initialise8259s, enableIRQ, disableIRQ, sendEOI"), None),
    ("arch/x86/common/source/KeyboardManager.cpp + atkbd.cpp", "keyboard IRQ: read scancodes, convert them to keys (" + c("convertScancode") + "), modifier state, LEDs; keys go to the Console thread", "Which file turns keyboard scancodes into keys?"),
    ("arch/common/include/KeyboardManager.h", c("KEY_F1 … KEY_F12") + " and the other key codes", "Where are the F-key codes defined?"),
    ("arch/x86/common/source/ATADriver.cpp + IDEDriver.cpp", "the disk drivers: IDE detection, partitions from the MBR (" + c("processMBR") + " → idea0/idea1/idea2), ATA sector read/write, IRQ handling", "Which files are the disk drivers?"),
    ("arch/x86/common/source/SerialManager.cpp + SerialPort.cpp", "serial ports (UART): detection, write, IRQ", None),
    ("arch/x86/common/source/assert.cpp", c("sweb_assert") + ": prints the failed condition + backtrace and halts – the KERNEL PANIC you see", None),
    ("arch/x86/common/source/debug_bochs.cpp", c("writeLine2Bochs") + ": early debug output on port 0xE9 (before the console exists)", None),
    ("arch/x86/common/include/ErrorHandlers.h", "the names of the 32 CPU exceptions (division by zero, GP fault, …) and their handler stubs", "Where are the CPU exception names (e.g. General Protection Fault)?"),
    ("arch/x86/common/include/ports.h", c("inportb/outportb") + " … – I/O port access", None),
    ("arch/x86/common/include/multiboot.h", "the multiboot structures GRUB passes to the kernel", None),
    # ---- console, debug output
    ("common/source/console/Console.cpp", c("handleKey") + " (F9–F12 debug output, Shift+Fn terminal switch – add your F-key here), " + c("Run") + " (the Console kernel thread), terminals", None),
    ("common/source/console/Terminal.cpp", "one terminal: input buffer (" + c("read, readLine") + "), output (" + c("write, writeString") + "), scrolling, colors", "Which file reads a line the user typed in the shell?"),
    ("common/source/console/TextConsole.cpp + FrameBufferConsole.cpp", "the two screen backends: VGA text mode and a VESA framebuffer (with " + c("font_sun8x16.c") + ")", None),
    ("common/source/console/kprintf.cpp", c("kprintf") + " (screen + debug) and " + c("kprintfd") + " (debug output only)", None),
    ("common/source/console/panic.cpp", c("kpanict") + ": print a message and stop", None),
    ("common/include/console/debug.h", "the debug flags (" + c("SYSCALL, PAGEFAULT, LOADER, USERPROCESS, …") + "), colors, " + c("debug(FLAG, …)") + " macro – add your own flag here", None),
    # ---- block devices and file systems
    ("common/source/fs/BDManager.cpp", "all block devices: detection, " + c("getDeviceByName / getDeviceByNumber") + ", IRQ dispatch", "Which file finds a disk by its name (idea2)?"),
    ("common/source/fs/BDVirtualDevice.cpp", "one disk/partition: " + c("readData(offset, size, buf), writeData") + ", block size, partition type", None),
    ("common/source/fs/VfsSyscall.cpp", "the file syscalls inside the kernel: " + c("open, close, read, write, lseek, mkdir, readdir, chdir, rm, rmdir, mount") + " – " + c("Syscall::open") + " etc. call these", "Which file implements open/read/write/mkdir for the syscalls?"),
    ("common/source/fs/VirtualFileSystem.cpp", "the VFS: registered file system types, " + c("rootMount, mount, umount"), None),
    ("common/source/fs/PathWalker.cpp + Path.cpp", "resolves a path string to dentries (" + c("pathWalk") + "): ., .., mount points, absolute/relative", None),
    ("common/source/fs/Inode.cpp, Dentry.cpp, Superblock.cpp, File.cpp, VfsMount.cpp", "the VFS objects: inode (a file/dir, link counts, mkdir/mkfile), dentry (a name in a directory), superblock (one mounted file system), File (an open file), mount", None),
    ("common/source/fs/FileDescriptor.cpp", c("FileDescriptorList") + ": the global fd table – " + c("add, remove, getFileDescriptor"), "Where are file descriptors kept?"),
    ("common/source/fs/minixfs/", "the Minix file system on idea1 (/usr: the user programs): superblock, inodes, zones, storage manager", None),
    ("common/source/fs/ramfs/ + devicefs/", "ramfs: the root file system in memory; devicefs: /dev (block and character devices as files)", None),
    # ---- util
    ("common/source/util/Bitmap.cpp", c("setBit, unsetBit, getBit, getNumBitsSet, getNumFreeBits") + " – used by the PageManager (and a swap manager)", "Which helper class tracks used/free slots with one bit each?"),
    ("common/source/util/kstring.cpp", "the kernel's string functions: " + c("memcpy, memset, memcmp, strlen, strcpy, strcmp, …") + " – user programs cannot use them", None),
    ("common/include/util/FiFo.h + RingBuffer.h", "templates for queues (keyboard input, terminal buffers)", None),
    ("common/source/util/Stabs2DebugInfo.cpp + SWEBDebugInfo.cpp", "debug symbols: function name + line for an address – what makes backtraces readable", None),
    ("common/source/util/qsort.cpp + libgcc.cpp", c("qsort") + " (heapsort) for the kernel; 64-bit division helpers the compiler calls", None),
    # ---- user space
    ("userspace/libc/src/nonstd.c + include/nonstd.h", c("_start") + " (" + c("exit(main())") + "), " + c("createprocess, getThreadCount, getProcessCount") + " – your own syscall wrappers go here", None),
    ("userspace/libc/src/stdlib.c", c("atexit") + " (works: " + c("atexit_functions[], __runAtexitFunctions") + "); "
     + c("malloc, free, calloc, realloc") + " – stubs", None),
    ("userspace/libc/src/exec.c", c("exit") + " (runs the atexit handlers, then the syscall), " + c("_exit, fork, execv"), None),
    ("userspace/libc/src/pthread.c", c("pthread_create") + " (+ the trampoline " + c("pthread_start") + "), " + c("exit, join, cancel, testcancel, self, equal, detach") + "; stubs: " + c("mutex_*, cond_*, spin_*, setcancelstate/type"), None),
    ("userspace/libc/src/semaphore.c", c("sem_init/wait/trywait/post/destroy") + " – all stubs", None),
    ("userspace/libc/src/unistd.c", c("getpid, getppid") + "; stubs " + c("brk, sbrk, sleep, ftruncate"), None),
    ("userspace/libc/src/time.c + wait.c + mman.c", c("clock") + ", " + c("waitpid") + ", " + c("mmap, munmap, shm_open, shm_unlink, mprotect") + " – all stubs", None),
    ("userspace/libc/src/string.c", c("memcmp, memcpy, memset, strlen, strcmp, strstr") + " – no strcpy/strncpy/strcat", None),
    ("userspace/libc/src/printf.c + scanf.c", c("printf, putchar, puts") + " (format in user space, then one " + c("write") + "); " + c("scanf, sscanf, getchar, gets, fgets, strtoul"), None),
    ("userspace/libc/src/read.c, write.c, file_alloc.c", "thin syscall wrappers: " + c("read, lseek, write, open, close, mkdir, link, unlink, pipe, dup, dup2, rename"), None),
    ("userspace/libc/src/sched.c + atoi.c + ctype.c", c("sched_yield") + " (syscall 158); " + c("atoi, atol") + "; " + c("isdigit, isalpha, …"), None),
    ("userspace/libc/include/types.h", c("size_t, ssize_t, pid_t, off_t, …") + " for user programs – include it where you use size_t", None),
    ("userspace/libc/include/sys/syscall.h", "the declaration of " + c("__syscall(number, a1, …, a5)"), None),
    ("userspace/tests/", "one program per .c file (→ /usr/name.sweb, built automatically): " + c("shell.c") + " (the shell), " + c("mult.c, memtest.c, helloworld.c") + ", your team's tests (" + c("pthread_*, fork_basic, exec_basic, mkdir_basic") + ")", None),
    ("userspace/tests/shell.c", "the shell: reads a line, runs " + c("name.sweb") + " with " + c("createprocess") + ", built-in commands " + c("ls/l, help/h, exit/q, cls/clear"), "Which program reads the commands you type after boot?"),
    # ---- small headers (rarely the target of a task, but you may land in them)
    ("arch/common/include/BDDriver.h + BDRequest.h", "the interface every disk driver implements (" + c("readSector, writeSector, addRequest") + ") and a request (" + c("BD_READ, BD_WRITE") + ", sector, buffer, status) – what " + c("BDVirtualDevice::readData") + " creates", None),
    ("arch/common/include/Elf64Format.h + Elf32Format.h", "the ELF structures (header, program header: " + c("p_type, p_flags (PF_X/PF_W/PF_R), p_vaddr, p_filesz, p_memsz") + ") – the loader reads them", "Where are the ELF program-header flags PF_X / PF_W defined?"),
    ("arch/common/include/ArchSerialInfo.h + arch/x86/common/source/arch_serial_constants.cpp", "port addresses and constants of the serial ports", None),
    ("common/include/fs/FileSystemInfo.h (+ .cpp)", "a thread's root and current directory (" + c("root_, pwd_") + ") – " + c("Thread::getWorkingDirInfo()") + ", copied by fork", "Where is a thread's current working directory stored?"),
    ("common/include/fs/FileSystemType.h (+ .cpp)", "one kind of file system (minixfs, ramfs, devicefs): name, flags, " + c("readSuper / createSuper"), None),
    ("common/include/fs/Dirent.h", "one directory entry as " + c("readdir") + " returns it (name, type)", None),
    ("common/include/util/chardev.h", c("CharacterDevice") + ": base of byte-stream devices (terminals, serial ports) – appears in /dev", None),
    ("common/include/util/stdarg.h + userspace/libc/include/stdarg.h", c("va_list, va_start, va_arg, va_end") + " – variadic functions like " + c("kprintf") + " / " + c("printf"), None),
    ("userspace/libc/include/fcntl.h", c("O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_APPEND") + " … – the flags for " + c("open"), "Where are the open() flags O_RDONLY / O_CREAT defined?"),
    ("userspace/libc/include/stdio.h", "the declarations of " + c("printf, putchar, puts, scanf, getchar, gets, fgets"), None),
    ("userspace/libc/include/sys/stat.h", "file mode bits for " + c("mkdir / creat"), None),
    ("userspace/libc/include/pthread.h", "types (" + c("pthread_t") + " = size_t; mutex/cond/spinlock = unsigned int), " + c("PTHREAD_CANCELED") + ", the declarations – " + c("pthread_spin_*") + " are NOT declared (add them)", "Where are pthread_t and pthread_mutex_t defined?"),
    ("userspace/libc/include/stdlib.h + unistd.h", c("malloc, free, calloc, realloc, atexit, exit, atoi") + " / " + c("getpid, getppid, fork, _exit, brk, sbrk, sleep, read, write, close") + " declarations", None),
    ("userspace/libc/include/wait.h + time.h + sched.h", c("waitpid") + " (" + c("WEXITED") + ") / " + c("clock, clock_t, CLOCKS_PER_SEC") + " (1000000) / " + c("sched_yield"), None),
    ("userspace/libc/include/sys/mman.h", c("PROT_NONE/READ/WRITE/EXEC, MAP_PRIVATE/SHARED/ANONYMOUS") + " and " + c("mmap, munmap, shm_open, shm_unlink, mprotect"), "Where are PROT_* and MAP_* defined?"),
    ("userspace/libc/include/semaphore.h + ctype.h", c("sem_t") + " (unsigned int) and the " + c("sem_*") + " declarations / " + c("isdigit, isalpha, toupper") + " …", None),
    ("the header of a .cpp file", c("common/source/X/Name.cpp") + " ↔ " + c("common/include/X/Name.h") + "; " + c("arch/x86/64/source/Name.cpp") + " ↔ " + c("arch/x86/64/include/") + " (64-bit only), " + c("arch/x86/common/include/") + " or " + c("arch/common/include/") + " (shared by all architectures) – CLion: Ctrl+Alt+Home switches between them", "Where is the header of a .cpp file?"),
    # ---- build and tools
    ("CMakeLists.txt (top) + arch/x86/64/CMakeLists.include", "the build: the disk image, " + c("make debug, make runcgdb, make rungdb, make mrproper") + " (top); " + c("make qemu, make kvm, make qemugdb") + " (x86-64 file)", "Where are the make targets qemu / kvm / qemugdb defined?"),
    ("utils/exe2minixfs/", "the tool that copies the built " + c(".sweb") + " programs onto the disk image (asserts when the image is full)", None),
    ("utils/images/menu.lst", "the GRUB menu (your team: " + c("timeout 0") + " – boots at once)", None),
    ("utils/gdbinit", "gdb settings for " + c("make runcgdb / rungdb") + " (connects to QEMU's gdb port)", None),
]

for path, what, reverse in FILES:
    DECKS.setdefault("2b Every file", [])
    DECKS["2b Every file"].append(("What is in " + c(path) + "?", what, "file"))
    if reverse:
        DECKS["2b Every file"].append((reverse, c(path) + " – " + what, "file reverse"))


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
