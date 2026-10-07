/*
 * Reference solution for Module 03, part A.
 *
 * Races in the original:
 *  - deposit/withdraw/transfer: balance += / -= is load-add-store; two
 *    threads updating the same account lose updates.
 *  - withdraw/transfer: "if (balance >= amount) balance -= amount" is
 *    check-then-act; two threads can both pass the check -> overdraft.
 *  - transfer: between "from -= amount" and "to += amount" the money is
 *    in neither account; a concurrent bank_total sees a wrong total.
 *  - total: sums while other threads modify -> a total that never existed.
 *  - balance: an unsynchronised read racing with writers (a data race).
 *
 * Fix: one mutex protecting all balances. Every function that touches a
 * balance holds it for the WHOLE check-and-update, so each operation is
 * atomic with respect to every other one. Argument checks that only read
 * n (never modified after init) need no lock.
 */
#include "bank.h"

#include <stdlib.h>

int bank_init(bank_t *b, size_t n, long initial)
{
  b->n = n;
  b->balance = malloc(n * sizeof *b->balance);
  if (!b->balance)
    return -1;
  for (size_t i = 0; i < n; i++)
    b->balance[i] = initial;
  pthread_mutex_init(&b->lock, NULL);
  return 0;
}

void bank_destroy(bank_t *b)
{
  pthread_mutex_destroy(&b->lock);
  free(b->balance);
  b->balance = NULL;
}

int bank_deposit(bank_t *b, size_t acc, long amount)
{
  if (acc >= b->n || amount <= 0)
    return -1;
  pthread_mutex_lock(&b->lock);
  b->balance[acc] += amount;
  pthread_mutex_unlock(&b->lock);
  return 0;
}

int bank_withdraw(bank_t *b, size_t acc, long amount)
{
  if (acc >= b->n || amount <= 0)
    return -1;
  int ret = -1;
  pthread_mutex_lock(&b->lock);
  if (b->balance[acc] >= amount) {   /* check and act under the same lock */
    b->balance[acc] -= amount;
    ret = 0;
  }
  pthread_mutex_unlock(&b->lock);    /* single exit point: never forget to unlock */
  return ret;
}

int bank_transfer(bank_t *b, size_t from, size_t to, long amount)
{
  if (from >= b->n || to >= b->n || from == to || amount <= 0)
    return -1;
  int ret = -1;
  pthread_mutex_lock(&b->lock);
  if (b->balance[from] >= amount) {
    b->balance[from] -= amount;
    b->balance[to] += amount;
    ret = 0;
  }
  pthread_mutex_unlock(&b->lock);
  return ret;
}

long bank_balance(bank_t *b, size_t acc)
{
  if (acc >= b->n)
    return 0;
  pthread_mutex_lock(&b->lock);
  long v = b->balance[acc];
  pthread_mutex_unlock(&b->lock);
  return v;
}

long bank_total(bank_t *b)
{
  long total = 0;
  pthread_mutex_lock(&b->lock);      /* nobody can change anything while we sum */
  for (size_t i = 0; i < b->n; i++)
    total += b->balance[i];
  pthread_mutex_unlock(&b->lock);
  return total;
}
