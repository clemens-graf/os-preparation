/* Reference solution for Module 06, part B (header). */
#pragma once
#include <pthread.h>

typedef struct {
  int n;
  pthread_mutex_t *forks;
} table_t;

int table_init(table_t *t, int n);
void table_destroy(table_t *t);
void pickup(table_t *t, int id);
void putdown(table_t *t, int id);
