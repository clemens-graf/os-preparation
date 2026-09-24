/*
 * callbacks.c - the "function pointer + context pointer" pattern.
 *
 * Library code that must call *your* code with *your* data takes two things:
 *   - a function pointer  (what to call)
 *   - a void * context    (what to pass to it)
 * qsort, bsearch, pthread_create, atexit-with-context, signal handlers,
 * SWEB's kernel threads ... all follow this shape.  If you understand this
 * file, pthread_create(&t, NULL, worker, &args) holds no mystery.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A generic "for each element" that knows nothing about the element type.
 * It walks the array in steps of elem_size bytes and hands every element
 * (plus the caller's context) to the callback. */
static void for_each(void *base, size_t count, size_t elem_size,
                     void (*fn)(void *elem, void *ctx), void *ctx)
{
  char *p = base;                         /* char * so that + moves by bytes */
  for (size_t i = 0; i < count; i++)
    fn(p + i * elem_size, ctx);
}

/* --- callback 1: sum up ints; the running total lives in the context --- */
static void add_int(void *elem, void *ctx)
{
  long *total = ctx;
  *total += *(int *)elem;
}

/* --- callback 2: count words longer than a limit; context is a struct --- */
struct long_word_query {
  size_t min_len;
  int hits;
};

static void count_long(void *elem, void *ctx)
{
  const char *word = *(const char **)elem;  /* element type is const char * */
  struct long_word_query *q = ctx;
  if (strlen(word) >= q->min_len)
    q->hits++;
}

/* --- qsort comparator: must return <0, 0, >0 like strcmp --- */
static int cmp_int_desc(const void *a, const void *b)
{
  int x = *(const int *)a, y = *(const int *)b;
  return (x < y) - (x > y);                /* avoids overflow of y - x */
}

int main(void)
{
  int numbers[] = {5, 3, 9, 1, 7};
  size_t n = sizeof numbers / sizeof numbers[0];

  long total = 0;
  for_each(numbers, n, sizeof numbers[0], add_int, &total);
  printf("sum = %ld\n", total);

  const char *words[] = {"thread", "fork", "semaphore", "pid", "scheduler"};
  struct long_word_query q = {.min_len = 6, .hits = 0};
  for_each(words, 5, sizeof words[0], count_long, &q);
  printf("words with >= %zu chars: %d\n", q.min_len, q.hits);

  qsort(numbers, n, sizeof numbers[0], cmp_int_desc);
  printf("sorted descending:");
  for (size_t i = 0; i < n; i++)
    printf(" %d", numbers[i]);
  printf("\n");
  return 0;
}
