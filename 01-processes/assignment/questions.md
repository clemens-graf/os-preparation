# Module 01 — Paper questions

Answer these on paper first, then check with a quick experiment if unsure.
Answers: `../../solutions/01-processes/answers.md`. Questions like these are
typical for OS exams and for the "explain this" part of an interview.

**Q1.** How many lines does this print?
```c
int main(void) { fork(); fork(); fork(); printf("x\n"); return 0; }
```

**Q2.** How many lines does this print?
```c
int main(void) {
  for (int i = 0; i < 3; i++)
    if (fork() == 0)
      break;
  printf("x\n");
  return 0;
}
```

**Q3.** How many processes exist in total (including the original) after
`if (fork() && fork()) fork();`?

**Q4.** What is printed, and in which order? Is the order guaranteed?
```c
int x = 1;
int main(void) {
  if (fork() == 0) { x = 2; printf("child %d\n", x); exit(0); }
  wait(NULL);
  printf("parent %d\n", x);
  return 0;
}
```

**Q5.** Run in a terminal, what does this print? Why?
```c
printf("A");
fork();
printf("B\n");
```

**Q6.** After a *successful* `execvp`, which of these are the same as before
the call: the PID, the open file descriptors, the heap contents, installed
signal *handlers*, *ignored* signals, the current working directory?

**Q7.** Why must `cd` be a shell builtin? Why is `exit` a builtin?

**Q8.** What is a zombie process, why does the kernel keep it around, and
what happens to a zombie whose parent terminates?

**Q9.** In `example/pipe_demo.c`, what happens if the `wc` child does not
close `fds[1]`? Describe precisely who waits for what.

**Q10.** (SWEB preview) `fork()` is called once but "returns twice": 0 in
the child, the child's PID in the parent. On the kernel side there is only
one system call being handled. How can the child's `fork()` return a
different value? What does the kernel have to set up for the child thread
so that it "returns" to user space at all?

**Q11.** What does `waitpid(-1, &status, WNOHANG)` do? When would a shell
use it instead of a blocking `waitpid(pid, &status, 0)`?
