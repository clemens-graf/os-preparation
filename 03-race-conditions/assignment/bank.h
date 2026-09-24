/*
 * bank.h - Module 03 assignment, part A.
 *
 * bank.c works perfectly - as long as only one thread uses it. Make it
 * thread-safe. You may (and will have to) add fields to bank_t.
 * All functions may be called concurrently from any number of threads.
 */
#pragma once
#include <pthread.h>
#include <stddef.h>

typedef struct {
  size_t n;          /* number of accounts            */
  long *balance;     /* balance[i] is account i       */
  /* TODO: whatever you need for thread safety */
} bank_t;

/* Create n accounts, each with `initial` money. 0 on success, -1 on error. */
int bank_init(bank_t *b, size_t n, long initial);
void bank_destroy(bank_t *b);

/* amount must be > 0 and acc valid, otherwise -1. */
int bank_deposit(bank_t *b, size_t acc, long amount);

/* Withdraw only if the balance suffices: a balance never becomes negative.
 * Returns 0 on success, -1 if not enough money or invalid arguments. */
int bank_withdraw(bank_t *b, size_t acc, long amount);

/* Move amount from one account to another, all-or-nothing: either both
 * balances change or neither does. -1 if not enough money in `from`,
 * invalid arguments or from == to. */
int bank_transfer(bank_t *b, size_t from, size_t to, long amount);

long bank_balance(bank_t *b, size_t acc);

/* Sum of all balances, as ONE consistent snapshot: the result must be a
 * total that actually existed at one moment in time. (Since transfers do
 * not change the total, a concurrent transfer must never be observed
 * "half done".) */
long bank_total(bank_t *b);
