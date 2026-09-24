# Module 01 — Answers

**Q1.** 8. Every `fork()` doubles the number of processes: 1 → 2 → 4 → 8,
and every process prints once.

**Q2.** 4. Only the original process keeps looping; each child breaks out
immediately. So the parent creates 3 children, and 3 children + 1 parent
print.

**Q3.** 4. The original P forks C1. In C1 the first `fork()` returned 0,
so `&&` short-circuits and C1 does nothing more. P (non-zero) evaluates the
second `fork()` and creates C2; in C2 it returned 0, so the condition is
false. P's condition is true and it forks C3. Processes: P, C1, C2, C3.

**Q4.** `child 2` then `parent 1`. The order is guaranteed by `wait(NULL)`:
the parent cannot print before the child has terminated. The parent prints
1 because the child changed its *own copy* of `x`.

**Q5.** `AB` twice (`AB\nAB\n`). "A" has no newline, so on a terminal it
is still in the line buffer when `fork()` copies the address space. Both
processes then append "B\n" and flush their own copy.

**Q6.** Same: PID, open file descriptors (unless opened with `O_CLOEXEC`),
ignored signals stay ignored, working directory. Gone: heap (the whole old
address space is replaced). Handled signals are reset to the default
action — the handler function's code no longer exists in the new program.

**Q7.** The working directory is per-process state. An external `cd`
would change the directory of the *child*, which then exits; the shell's
own directory would be unchanged. The same holds for `exit`: only the shell
itself can terminate the shell.

**Q8.** A zombie is a process that has terminated but whose exit status has
not been collected by its parent via `wait`. The kernel keeps the PID and
exit status (not the memory) so the parent can still ask for them. If the
parent terminates, the zombie is re-parented to init (or a subreaper), which
waits for it and thereby removes it.

**Q9.** `wc` reads from the pipe until `read()` returns 0 (end-of-file).
The kernel only reports EOF when *every* write end of the pipe is closed.
`wc` itself holds a write end, so after `ls` has exited and the parent has
closed its copy, one write end is still open — owned by the reader. `wc`
blocks in `read()` forever, and the parent blocks in `waitpid(reader)`
forever. Deadlock by an unclosed file descriptor.

**Q10.** The kernel builds a new thread for the child whose *user register
set* is a copy of the parent's saved user registers (the state at the
moment of the syscall trap), with one change: the return-value register
(`rax` on x86-64) is set to 0. The parent's syscall returns normally with
the child's PID in `rax`. When the scheduler later runs the child thread,
it "returns from the interrupt" using the copied register set, so it
resumes at the instruction after the syscall — in the child's own copy of
the address space, with a copy of the user stack — and sees 0. So the kernel
must set up: a new address space (copy or copy-on-write of the parent's),
a new kernel stack, a register set that resumes in user mode, and it must
add the thread to the scheduler.

**Q11.** It reaps *any* child that has already terminated, without
blocking; it returns 0 if children exist but none has terminated yet. A
shell uses it for background jobs (`cmd &`): it must not block, but should
still collect finished jobs, typically in its main loop or triggered by a
`SIGCHLD` handler.
