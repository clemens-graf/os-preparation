/*
 * toolbox.h - Module 00 assignment.  Implement everything in toolbox.c.
 *
 * Three small parts, each training one skill you need for threads:
 *   A) vec_*   : heap memory, growing arrays, ownership (who frees what)
 *   B) task_*  : function pointer + void * argument + out-parameters, i.e.
 *                exactly the shape of pthread_create / pthread_join,
 *                just without any threads yet
 *   C) split_words : string handling + returning heap arrays (you reuse the
 *                idea in module 01 for the mini shell)
 */
#pragma once
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Part A: a growable array of void * items                            */
/* ------------------------------------------------------------------ */
typedef struct {
  void **items;   /* heap array with room for `cap` pointers           */
  size_t len;     /* number of used slots                              */
  size_t cap;     /* number of allocated slots                         */
} vec_t;

/* Initialise an empty vector (no allocation needed yet). Returns 0. */
int vec_init(vec_t *v);

/* Append item. Grow the storage by doubling when full (start with 4).
 * Returns 0 on success, -1 if memory allocation failed (v unchanged). */
int vec_push(vec_t *v, void *item);

/* Item at index i, or NULL if i is out of range. */
void *vec_get(const vec_t *v, size_t i);

/* Remove and return the last item, or NULL if the vector is empty. */
void *vec_pop(vec_t *v);

/* Call fn(item, ctx) for every item, in index order. */
void vec_foreach(const vec_t *v, void (*fn)(void *item, void *ctx), void *ctx);

/* Release the storage. If free_item is not NULL, call it on every item
 * first (the vector "owns" its items then). Leaves v as an empty vector. */
void vec_free(vec_t *v, void (*free_item)(void *item));

/* ------------------------------------------------------------------ */
/* Part B: deferred function calls ("a thread without the thread")     */
/* ------------------------------------------------------------------ */
typedef void *(*task_fn)(void *arg);

typedef struct task {
  task_fn fn;     /* what to run                                       */
  void *arg;      /* what to pass to it                                */
  void *result;   /* what fn returned (valid once done != 0)           */
  int done;       /* has task_run() completed?                         */
} task_t;

/* Allocate a task on the heap that will later call fn(arg), and store the
 * handle in *out.  Returns 0 on success, -1 if out or fn is NULL or
 * allocation fails (then *out is left untouched). */
int task_create(task_t **out, task_fn fn, void *arg);

/* Run the task: store fn(arg) in result and mark it done. */
void task_run(task_t *t);

/* Collect the result and destroy the task.
 * - If the task has not run yet: return -1 and do NOT free it.
 * - Otherwise: if result_out != NULL store the result there,
 *   free the task and return 0. */
int task_join(task_t *t, void **result_out);

/* ------------------------------------------------------------------ */
/* Part C: splitting a line into words                                 */
/* ------------------------------------------------------------------ */

/* Split `line` at spaces, tabs and newlines. Returns a heap-allocated,
 * NULL-terminated array of heap-allocated copies of the words (the exact
 * format that execvp() wants in module 01), and stores the number of
 * words in *count (if count != NULL).
 * "  ls   -l \n" -> {"ls", "-l", NULL}, count 2.
 * An empty/blank line gives {NULL}, count 0.
 * `line` itself must not be modified. Returns NULL on allocation failure. */
char **split_words(const char *line, size_t *count);

/* Free an array returned by split_words (NULL is allowed). */
void free_words(char **words);
