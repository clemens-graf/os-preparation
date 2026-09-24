/*
 * lifetime.c - where does memory live, and how long?
 *
 *   static / global : exists for the whole program run   (data / bss segment)
 *   local variable  : exists until the function returns (stack)
 *   malloc'd memory : exists until you free() it         (heap)
 *
 * Most threading bugs you will meet in the next modules are lifetime bugs
 * in disguise: a thread keeps a pointer to something that no longer exists.
 *
 *   make run          -> the correct variants
 *   make bug-stack    -> uses a pointer to a dead stack frame (ASan catches it)
 *   make bug-uaf      -> uses heap memory after free()        (ASan catches it)
 *   make bug-leak     -> forgets to free()                    (LeakSanitizer catches it)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int counter_global = 0;             /* lives forever                  */

static int *next_id_static(void)
{
  static int id = 100;                     /* 'static' local: lives forever, */
  id++;                                    /* but only visible in here       */
  return &id;                              /* OK: the object outlives us     */
}

/* WRONG: x dies when this function returns.  The pointer written to *out
 * dangles.  The compiler even warns if you `return &x;` directly - with an
 * out-parameter it cannot see the problem, but AddressSanitizer can. */
static void broken_make_value(int **out)
{
  int x = 42;
  *out = &x;
}

/* RIGHT: heap memory survives the return; the caller must free it. */
static int *make_value(void)
{
  int *x = malloc(sizeof *x);
  if (x)
    *x = 42;
  return x;
}

int main(int argc, char **argv)
{
  const char *mode = argc > 1 ? argv[1] : "ok";

  if (strcmp(mode, "ok") == 0) {
    counter_global++;
    printf("global counter  = %d\n", counter_global);
    printf("static local id = %d\n", *next_id_static());
    printf("static local id = %d (same object, incremented)\n", *next_id_static());

    int *v = make_value();
    printf("heap value      = %d\n", *v);
    free(v);                               /* every malloc gets one free     */
    v = NULL;                              /* habit: no dangling pointer left */
    return 0;
  }

  if (strcmp(mode, "stack") == 0) {
    int *p;
    broken_make_value(&p);
    /* ASan reports stack-use-after-scope (the call got inlined) or
     * stack-use-after-return - both mean "that stack variable is dead". */
    printf("reading a dead stack variable: %d\n", *p);
  } else if (strcmp(mode, "uaf") == 0) {
    int *v = make_value();
    free(v);
    printf("reading freed heap memory: %d\n", *v);       /* ASan: heap-use-after-free */
  } else if (strcmp(mode, "leak") == 0) {
    int *v = make_value();
    printf("leaking %p\n", (void *)v);                    /* LSan: detected memory leak */
    v = NULL;
  }
  return 0;
}
