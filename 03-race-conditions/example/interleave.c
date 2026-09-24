/*
 * interleave.c - enumerate EVERY possible interleaving of two tiny threads.
 *
 * Real races are hard to study because the scheduler picks one random
 * interleaving per run. Here a toy machine executes two "threads"
 * instruction by instruction, and we try every possible order. Each thread
 * has a private register r; memory (x or balance) is shared.
 *
 * Scenario 1:  both threads run  x = x + 1        (x starts at 0)
 * Scenario 2:  both threads run  if (balance >= 80) balance -= 80;
 *              (balance starts at 100 - only ONE withdrawal should succeed)
 *
 * Interleavings of two sequences of length m and n: (m+n)! / (m! n!).
 * Scenario 1: 6!/(3!3!) = 20. Only a handful of them are correct.
 */
#include <stdio.h>
#include <string.h>

enum opcode { LOAD, ADDI, STORE, JLT, DONE };

struct instr {
  enum opcode op;
  int imm;        /* ADDI: value to add; JLT: compare value */
  int target;     /* JLT: jump here if r < imm */
};

struct thread {
  const struct instr *prog;
  int pc;
  int r;          /* private register */
  int stores;     /* how many STOREs this thread executed */
};

/* ------------------------------------------------------------------ */
static const struct instr increment_prog[] = {
  {LOAD, 0, 0},           /* r = x      */
  {ADDI, 1, 0},           /* r = r + 1  */
  {STORE, 0, 0},          /* x = r      */
  {DONE, 0, 0},
};

static const struct instr withdraw_prog[] = {
  {LOAD, 0, 0},           /* 0: r = balance              */
  {JLT, 80, 5},           /* 1: if (r < 80) goto 5       */
  {LOAD, 0, 0},           /* 2: r = balance  (re-read!)  */
  {ADDI, -80, 0},         /* 3: r = r - 80               */
  {STORE, 0, 0},          /* 4: balance = r              */
  {DONE, 0, 0},           /* 5: */
};

static const char *names[] = {"LOAD", "ADDI", "STORE", "JLT", "DONE"};

/* ------------------------------------------------------------------ */
#define MAX_STEPS 32
#define MAX_OUTCOMES 16

struct outcome {
  int mem, stores, count;
  char example[256];
};

static struct outcome outcomes[MAX_OUTCOMES];
static int n_outcomes, n_schedules, print_all;

static void record(int mem, int stores, const char *schedule)
{
  n_schedules++;
  if (print_all)
    printf("  %-40s -> %d\n", schedule, mem);
  for (int i = 0; i < n_outcomes; i++)
    if (outcomes[i].mem == mem && outcomes[i].stores == stores) {
      outcomes[i].count++;
      return;
    }
  struct outcome *o = &outcomes[n_outcomes++];
  o->mem = mem;
  o->stores = stores;
  o->count = 1;
  snprintf(o->example, sizeof o->example, "%s", schedule);
}

/* Execute one instruction of thread t. */
static void step(struct thread *t, int *mem)
{
  const struct instr *in = &t->prog[t->pc];
  switch (in->op) {
  case LOAD:  t->r = *mem; t->pc++; break;
  case ADDI:  t->r += in->imm; t->pc++; break;
  case STORE: *mem = t->r; t->stores++; t->pc++; break;
  case JLT:   t->pc = t->r < in->imm ? in->target : t->pc + 1; break;
  case DONE:  break;
  }
}

/* Depth-first search over all choices "which thread runs next". The whole
 * machine state is passed by value, so backtracking is free. */
static void explore(struct thread a, struct thread b, int mem, char *sched, size_t len)
{
  int a_done = a.prog[a.pc].op == DONE, b_done = b.prog[b.pc].op == DONE;
  if (a_done && b_done) {
    record(mem, a.stores + b.stores, sched);
    return;
  }
  if (!a_done) {
    struct thread a2 = a;
    int m2 = mem;
    int n = snprintf(sched + len, 256 - len, "A:%s ", names[a.prog[a.pc].op]);
    step(&a2, &m2);
    explore(a2, b, m2, sched, len + (size_t)n);
    sched[len] = '\0';
  }
  if (!b_done) {
    struct thread b2 = b;
    int m2 = mem;
    int n = snprintf(sched + len, 256 - len, "B:%s ", names[b.prog[b.pc].op]);
    step(&b2, &m2);
    explore(a, b2, m2, sched, len + (size_t)n);
    sched[len] = '\0';
  }
}

static void run(const char *title, const struct instr *prog, int initial, int show_all)
{
  char sched[256] = "";
  n_outcomes = n_schedules = 0;
  print_all = show_all;
  printf("=== %s ===\n", title);
  struct thread a = {prog, 0, 0, 0}, b = {prog, 0, 0, 0};
  explore(a, b, initial, sched, 0);
  printf("%d interleavings, %d distinct outcome(s):\n", n_schedules, n_outcomes);
  for (int i = 0; i < n_outcomes; i++)
    printf("  final value %4d, %d successful store(s): %3d schedule(s), e.g. %s\n",
           outcomes[i].mem, outcomes[i].stores, outcomes[i].count, outcomes[i].example);
  printf("\n");
}

int main(void)
{
  run("Scenario 1: two threads do x = x + 1, x starts at 0", increment_prog, 0, 1);
  printf("Correct result is 2. Every schedule where one thread's LOAD falls between\n"
         "the other thread's LOAD and STORE loses an update.\n\n");

  run("Scenario 2: two threads do if (balance >= 80) balance -= 80, start 100",
      withdraw_prog, 100, 0);
  printf("Correct: balance 20 with ONE store. Also possible:\n"
         " - balance -60: both passed the check before either withdrew (check-then-act race)\n"
         " - balance 20 with TWO stores: both withdrew 80, one update was lost - the bank\n"
         "   paid out 160 but the balance only shows 80 missing (lost update)\n");
  return 0;
}
