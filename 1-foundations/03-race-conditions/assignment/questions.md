# Module 03 — Paper questions

Answers: `../../solutions/03-race-conditions/answers.md`.

**Q1.** Define *data race* and *race condition*. Give an example of a race
condition that contains no data race (every single access is atomic or
locked, and the program is still wrong).

**Q2.** Thread A executes 2 instructions, thread B executes 3. How many
different interleavings exist? And for 3 threads with 2 instructions each?

**Q3.** `x` starts at 0. Three threads each execute `x++` (as LOAD/ADD/STORE)
once. Which final values of `x` are possible?

**Q4.** SWEB normally runs on a single CPU core. Can `counter++` from two
threads still lose updates there? What exactly has to happen?

**Q5.** Why does declaring the counter `volatile` not fix the race in
`example/lost_update.c`?

**Q6.** In module 02 you read `partial[i]` after `pthread_join` without any
lock. Why is that not a data race?

**Q7.** In `bank.c`, why is it not enough to protect each account's balance
with its own atomic variable (`atomic_long balance[i]`) for `bank_transfer`
and `bank_total`?

**Q8.** A colleague "fixes" the lazy initialisation like this:
```c
if (config == NULL) {
  pthread_mutex_lock(&m);
  config = load_config();
  pthread_mutex_unlock(&m);
}
```
What is still wrong? Fix it.
