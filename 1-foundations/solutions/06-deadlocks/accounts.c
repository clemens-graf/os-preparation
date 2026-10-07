/*
 * Reference solution for Module 06, part A.
 *
 * Deadlock in the original: T1 transfers 0 -> 1 (locks 0, waits for 1)
 * while T2 transfers 1 -> 0 (locks 1, waits for 0): circular wait. And
 * from == to locked the same (non-recursive) mutex twice: a thread
 * deadlocking with itself.
 *
 * Fix: break the "circular wait" condition with a global lock order -
 * always lock the account with the smaller index first. Every thread
 * then acquires locks in increasing index order, so no cycle can form in
 * the wait-for graph. accounts_total already locked in increasing order,
 * so it fits the same rule.
 */
#include "accounts.h"

#include <stdlib.h>

int accounts_init(accounts_t *a, size_t n, long initial)
{
  a->n = n;
  a->acc = calloc(n, sizeof *a->acc);
  if (!a->acc)
    return -1;
  for (size_t i = 0; i < n; i++) {
    pthread_mutex_init(&a->acc[i].lock, NULL);
    a->acc[i].balance = initial;
  }
  return 0;
}

void accounts_destroy(accounts_t *a)
{
  for (size_t i = 0; i < a->n; i++)
    pthread_mutex_destroy(&a->acc[i].lock);
  free(a->acc);
  a->acc = NULL;
}

int accounts_transfer(accounts_t *a, size_t from, size_t to, long amount)
{
  if (from >= a->n || to >= a->n || from == to || amount <= 0)
    return -1;

  size_t first = from < to ? from : to;
  size_t second = from < to ? to : from;
  pthread_mutex_lock(&a->acc[first].lock);
  pthread_mutex_lock(&a->acc[second].lock);

  int ret = -1;
  if (a->acc[from].balance >= amount) {
    a->acc[from].balance -= amount;
    a->acc[to].balance += amount;
    ret = 0;
  }

  pthread_mutex_unlock(&a->acc[second].lock);  /* unlock order does not matter */
  pthread_mutex_unlock(&a->acc[first].lock);   /* for deadlocks, only lock order */
  return ret;
}

long accounts_total(accounts_t *a)
{
  /* increasing index order: consistent with accounts_transfer */
  for (size_t i = 0; i < a->n; i++)
    pthread_mutex_lock(&a->acc[i].lock);
  long total = 0;
  for (size_t i = 0; i < a->n; i++)
    total += a->acc[i].balance;
  for (size_t i = 0; i < a->n; i++)
    pthread_mutex_unlock(&a->acc[i].lock);
  return total;
}
