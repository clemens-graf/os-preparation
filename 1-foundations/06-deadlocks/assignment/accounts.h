/*
 * accounts.h - Module 06 assignment, part A.
 *
 * Module 03's bank used ONE lock: correct, but all transfers are
 * serialised even when they touch different accounts. Here every account
 * has its own mutex so independent transfers run in parallel.
 * The provided accounts.c does the obvious thing - and deadlocks.
 */
#pragma once
#include <pthread.h>
#include <stddef.h>

typedef struct {
  pthread_mutex_t lock;
  long balance;
} account_t;

typedef struct {
  size_t n;
  account_t *acc;
} accounts_t;

int accounts_init(accounts_t *a, size_t n, long initial);
void accounts_destroy(accounts_t *a);

/* Move amount from -> to, all-or-nothing. Returns 0, or -1 if the funds
 * are insufficient or the arguments are invalid (from == to is invalid).
 * Must hold BOTH account locks while checking and moving the money, and
 * must never deadlock, whatever other transfers run concurrently. */
int accounts_transfer(accounts_t *a, size_t from, size_t to, long amount);

/* Consistent total over all accounts (hold all locks at once while
 * summing). Must not deadlock with concurrent transfers either. */
long accounts_total(accounts_t *a);
