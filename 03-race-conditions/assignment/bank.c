/*
 * bank.c - Module 03 assignment, part A.
 *
 * This code is correct single-threaded and has several race conditions.
 * Your job:
 *   1. Find every race. Write a short comment at each one: which two
 *      operations can interleave badly and what goes wrong
 *      (lost update? check-then-act? inconsistent snapshot?).
 *   2. Fix them. Simplest correct design first: one mutex for the whole
 *      bank. (Per-account locks come in module 06 - with a surprise.)
 *
 * Test:  make test     (runs under ThreadSanitizer and at full speed)
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
  return 0;
}

void bank_destroy(bank_t *b)
{
  free(b->balance);
  b->balance = NULL;
}

int bank_deposit(bank_t *b, size_t acc, long amount)
{
  if (acc >= b->n || amount <= 0)
    return -1;
  b->balance[acc] += amount;
  return 0;
}

int bank_withdraw(bank_t *b, size_t acc, long amount)
{
  if (acc >= b->n || amount <= 0)
    return -1;
  if (b->balance[acc] < amount)
    return -1;
  b->balance[acc] -= amount;
  return 0;
}

int bank_transfer(bank_t *b, size_t from, size_t to, long amount)
{
  if (from >= b->n || to >= b->n || from == to || amount <= 0)
    return -1;
  if (b->balance[from] < amount)
    return -1;
  b->balance[from] -= amount;
  b->balance[to] += amount;
  return 0;
}

long bank_balance(bank_t *b, size_t acc)
{
  if (acc >= b->n)
    return 0;
  return b->balance[acc];
}

long bank_total(bank_t *b)
{
  long total = 0;
  for (size_t i = 0; i < b->n; i++)
    total += b->balance[i];
  return total;
}
