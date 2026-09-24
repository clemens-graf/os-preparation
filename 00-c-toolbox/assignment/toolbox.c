/*
 * toolbox.c - Module 00 assignment.  Replace every TODO.
 * Build & test:  make test
 *
 * Rules: plain C, no global variables. Every malloc must have exactly one
 * matching free somewhere - the tests run under AddressSanitizer and
 * LeakSanitizer, which will point at any leak or invalid access.
 */
#include "toolbox.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------ Part A ------------------------------ */

int vec_init(vec_t *v)
{
  /* TODO: set up an empty vector. */
  return -1;
}

int vec_push(vec_t *v, void *item)
{
  /* TODO: grow (start with capacity 4, then double) and append.
   * Hint: realloc(NULL, n) behaves like malloc(n). Assign realloc's result
   * to a temporary first - if it returns NULL, the old block is still yours. */
  return -1;
}

void *vec_get(const vec_t *v, size_t i)
{
  /* TODO */
  return NULL;
}

void *vec_pop(vec_t *v)
{
  /* TODO */
  return NULL;
}

void vec_foreach(const vec_t *v, void (*fn)(void *item, void *ctx), void *ctx)
{
  /* TODO */
}

void vec_free(vec_t *v, void (*free_item)(void *item))
{
  /* TODO */
}

/* ------------------------------ Part B ------------------------------ */

int task_create(task_t **out, task_fn fn, void *arg)
{
  /* TODO */
  return -1;
}

void task_run(task_t *t)
{
  /* TODO */
}

int task_join(task_t *t, void **result_out)
{
  /* TODO */
  return -1;
}

/* ------------------------------ Part C ------------------------------ */

char **split_words(const char *line, size_t *count)
{
  /* TODO: suggestion - first count the words, then allocate count + 1
   * pointers, then copy each word with malloc + memcpy (or strndup). */
  return NULL;
}

void free_words(char **words)
{
  /* TODO */
}
