/*
 * accounts.c - Module 06 assignment, part A.
 *
 * This version is correct as long as it does not deadlock - which it
 * does, as soon as one thread transfers 1 -> 2 while another transfers
 * 2 -> 1. Fix accounts_transfer (and check accounts_total) so that no
 * deadlock is possible. Write a comment naming the Coffman condition you
 * break.
 *
 * Test:  make test-accounts
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
  if (from >= a->n || to >= a->n || amount <= 0)
    return -1;

  pthread_mutex_lock(&a->acc[from].lock);
  pthread_mutex_lock(&a->acc[to].lock);

  int ret = -1;
  if (a->acc[from].balance >= amount) {
    a->acc[from].balance -= amount;
    a->acc[to].balance += amount;
    ret = 0;
  }

  pthread_mutex_unlock(&a->acc[to].lock);
  pthread_mutex_unlock(&a->acc[from].lock);
  return ret;
}

long accounts_total(accounts_t *a)
{
  for (size_t i = 0; i < a->n; i++)
    pthread_mutex_lock(&a->acc[i].lock);
  long total = 0;
  for (size_t i = 0; i < a->n; i++)
    total += a->acc[i].balance;
  for (size_t i = 0; i < a->n; i++)
    pthread_mutex_unlock(&a->acc[i].lock);
  return total;
}
