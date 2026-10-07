/*
 * Reference solution for Module 00.  Try the assignment yourself first -
 * comparing afterwards teaches far more than reading this first.
 */
#include "toolbox.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------ Part A ------------------------------ */

int vec_init(vec_t *v)
{
  v->items = NULL;
  v->len = 0;
  v->cap = 0;
  return 0;
}

int vec_push(vec_t *v, void *item)
{
  if (v->len == v->cap) {
    size_t new_cap = v->cap ? v->cap * 2 : 4;
    /* Never write `v->items = realloc(v->items, ...)`: on failure realloc
     * returns NULL and the old block would be leaked. */
    void **bigger = realloc(v->items, new_cap * sizeof *bigger);
    if (!bigger)
      return -1;
    v->items = bigger;
    v->cap = new_cap;
  }
  v->items[v->len++] = item;
  return 0;
}

void *vec_get(const vec_t *v, size_t i)
{
  return i < v->len ? v->items[i] : NULL;   /* size_t is unsigned: no i < 0 case */
}

void *vec_pop(vec_t *v)
{
  return v->len ? v->items[--v->len] : NULL;
}

void vec_foreach(const vec_t *v, void (*fn)(void *item, void *ctx), void *ctx)
{
  for (size_t i = 0; i < v->len; i++)
    fn(v->items[i], ctx);
}

void vec_free(vec_t *v, void (*free_item)(void *item))
{
  if (free_item)
    for (size_t i = 0; i < v->len; i++)
      free_item(v->items[i]);
  free(v->items);
  vec_init(v);
}

/* ------------------------------ Part B ------------------------------ */

int task_create(task_t **out, task_fn fn, void *arg)
{
  if (!out || !fn)
    return -1;
  task_t *t = malloc(sizeof *t);
  if (!t)
    return -1;
  t->fn = fn;
  t->arg = arg;
  t->result = NULL;
  t->done = 0;
  *out = t;          /* only publish the handle once it is fully set up */
  return 0;
}

void task_run(task_t *t)
{
  t->result = t->fn(t->arg);
  t->done = 1;
}

int task_join(task_t *t, void **result_out)
{
  if (!t->done)
    return -1;
  if (result_out)
    *result_out = t->result;
  free(t);
  return 0;
}

/* ------------------------------ Part C ------------------------------ */

static int is_sep(char c)
{
  return c == ' ' || c == '\t' || c == '\n';
}

char **split_words(const char *line, size_t *count)
{
  size_t n = 0;
  for (const char *p = line; *p;) {
    while (*p && is_sep(*p))
      p++;
    if (!*p)
      break;
    n++;
    while (*p && !is_sep(*p))
      p++;
  }

  char **words = malloc((n + 1) * sizeof *words);
  if (!words)
    return NULL;

  size_t i = 0;
  for (const char *p = line; *p;) {
    while (*p && is_sep(*p))
      p++;
    if (!*p)
      break;
    const char *start = p;
    while (*p && !is_sep(*p))
      p++;
    words[i] = strndup(start, (size_t)(p - start));
    if (!words[i]) {
      words[i] = NULL;
      free_words(words);
      return NULL;
    }
    i++;
  }
  words[n] = NULL;
  if (count)
    *count = n;
  return words;
}

void free_words(char **words)
{
  if (!words)
    return;
  for (char **w = words; *w; w++)
    free(*w);
  free(words);
}
