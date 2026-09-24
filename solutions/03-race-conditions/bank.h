/* Reference solution for Module 03, part A (header). */
#pragma once
#include <pthread.h>
#include <stddef.h>

typedef struct {
  size_t n;          /* number of accounts            */
  long *balance;     /* balance[i] is account i       */
  pthread_mutex_t lock;   /* protects every balance[i]; n and the pointer never change */
} bank_t;

int bank_init(bank_t *b, size_t n, long initial);
void bank_destroy(bank_t *b);
int bank_deposit(bank_t *b, size_t acc, long amount);
int bank_withdraw(bank_t *b, size_t acc, long amount);
int bank_transfer(bank_t *b, size_t from, size_t to, long amount);
long bank_balance(bank_t *b, size_t acc);
long bank_total(bank_t *b);
