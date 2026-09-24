/*
 * signals.c - signals are "software interrupts" delivered to a process.
 *
 * The kernel can interrupt a process at (almost) any instruction and run a
 * handler function, then resume where it left off. That is exactly how a
 * timer interrupt preempts a thread - you will use this in module 09/10.
 *
 * The rule that follows from "at any instruction": a handler may run in
 * the middle of printf(), malloc(), or a half-finished update of your
 * data. So a handler may only touch `volatile sig_atomic_t` flags and call
 * async-signal-safe functions (write, _exit, ...; see `man 7 signal-safety`).
 * This is the same reasoning as "interrupt handlers must not take locks the
 * interrupted code may hold" in a kernel.
 *
 * Run it and press Ctrl+C a few times (or wait 5 s).
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t interrupts = 0;
static volatile sig_atomic_t ticks = 0;

static void on_sigint(int sig)
{
  (void)sig;
  interrupts++;
  const char msg[] = "  [handler] got SIGINT\n";
  ssize_t unused = write(STDOUT_FILENO, msg, sizeof msg - 1);  /* write is async-signal-safe, printf is not */
  (void)unused;
}

static void on_alarm(int sig)
{
  (void)sig;
  ticks++;
  alarm(1);                         /* re-arm: one "timer interrupt" per second */
}

int main(void)
{
  struct sigaction sa;
  memset(&sa, 0, sizeof sa);
  sa.sa_handler = on_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;         /* restart interrupted syscalls like read() */
  sigaction(SIGINT, &sa, NULL);

  sa.sa_handler = on_alarm;
  sigaction(SIGALRM, &sa, NULL);
  alarm(1);

  printf("PID %d: press Ctrl+C (3 times to quit), or wait 5 timer ticks\n", getpid());
  while (interrupts < 3 && ticks < 5) {
    pause();                        /* sleep until any signal arrives */
    printf("main loop woke up: interrupts=%d ticks=%d\n", (int)interrupts, (int)ticks);
  }
  return 0;
}
