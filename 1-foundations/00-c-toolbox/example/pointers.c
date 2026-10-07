/*
 * pointers.c - a guided tour of the pointer features that systems code uses
 * all the time.  Read the source top to bottom, then run it (`make run`)
 * and compare what is printed with the comments.
 *
 * The goal of this file: by the end you can read this declaration fluently,
 * because it is the one you will implement in SWEB as P1:
 *
 *   int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
 *                      void *(*start_routine)(void *), void *arg);
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* 1. A pointer is just a variable that holds an address.              */
/* ------------------------------------------------------------------ */
static void part1_basics(void)
{
  int x = 42;
  int *p = &x;          /* p holds the address of x                    */

  printf("1) x = %d, &x = %p, p = %p, *p = %d\n", x, (void *)&x, (void *)p, *p);

  *p = 7;               /* writing through the pointer changes x       */
  printf("   after *p = 7: x = %d\n", x);

  /* sizeof(p) is the size of an address (8 bytes on x86-64), no matter
   * what it points to.  sizeof(*p) is the size of the pointed-to thing. */
  printf("   sizeof(p) = %zu, sizeof(*p) = %zu\n", sizeof(p), sizeof(*p));
}

/* ------------------------------------------------------------------ */
/* 2. Out-parameters: a function can only "return" extra values by    */
/*    writing through a pointer the caller gives it.                   */
/*    pthread_create(pthread_t *thread, ...) uses exactly this: the    */
/*    new thread's id is written to *thread.                           */
/* ------------------------------------------------------------------ */
static int divide(int a, int b, int *quotient, int *remainder)
{
  if (b == 0)
    return -1;          /* error code as return value ...              */
  *quotient = a / b;    /* ... and the real results via out-params     */
  *remainder = a % b;
  return 0;
}

static void part2_out_params(void)
{
  int q, r;
  if (divide(17, 5, &q, &r) == 0)
    printf("2) 17 / 5 = %d remainder %d\n", q, r);
}

/* ------------------------------------------------------------------ */
/* 3. Pointer to pointer: needed when the function has to hand back a  */
/*    *pointer*.  pthread_join(pthread_t t, void **retval) writes the  */
/*    thread's return value (a void *) into *retval.                   */
/* ------------------------------------------------------------------ */
static int make_greeting(const char *name, char **out)
{
  char *buf = malloc(64);
  if (!buf)
    return -1;
  snprintf(buf, 64, "hello, %s", name);
  *out = buf;           /* the caller's pointer now points at our heap buffer */
  return 0;
}

static void part3_pointer_to_pointer(void)
{
  char *greeting = NULL;
  if (make_greeting("SWEB", &greeting) == 0) {
    printf("3) %s\n", greeting);
    free(greeting);     /* whoever receives heap memory must free it */
  }
}

/* ------------------------------------------------------------------ */
/* 4. void * is "a pointer to something, type unknown".                */
/*    Any object pointer converts to void * and back without a cast.   */
/*    You cannot dereference a void * - you must convert it to the     */
/*    real type first.  Generic APIs (qsort, pthread_create) use it to */
/*    pass *your* data through code that does not know your types.     */
/* ------------------------------------------------------------------ */
struct point { int x, y; };

static void print_anything(void *thing, char kind)
{
  if (kind == 'i') {
    int *ip = thing;                  /* convert back to the real type */
    printf("4) int: %d\n", *ip);
  } else if (kind == 'p') {
    struct point *pt = thing;
    printf("4) point: (%d, %d)\n", pt->x, pt->y);   /* a->b == (*a).b */
  }
}

static void part4_void_pointers(void)
{
  int n = 5;
  struct point pt = {3, 4};
  print_anything(&n, 'i');
  print_anything(&pt, 'p');

  /* Small integers are sometimes smuggled *inside* the pointer value
   * instead of pointing at them.  Always go through intptr_t, never int. */
  void *packed = (void *)(intptr_t)1234;
  int unpacked = (int)(intptr_t)packed;
  printf("4) integer packed into a void *: %d\n", unpacked);
}

/* ------------------------------------------------------------------ */
/* 5. Function pointers.  A function's name used without () is its    */
/*    address.  Read declarations "from the name outwards":            */
/*                                                                     */
/*    void *(*start_routine)(void *)                                   */
/*           ^^^^^^^^^^^^^           start_routine is ...              */
/*          (*             )         ... a pointer to ...              */
/*                          (void *) ... a function taking a void * ...*/
/*    void *                         ... and returning a void *.       */
/* ------------------------------------------------------------------ */
typedef void *(*thread_fn)(void *);   /* a typedef makes it readable     */

static void *square(void *arg)
{
  long v = *(long *)arg;
  long *result = malloc(sizeof *result);
  *result = v * v;
  return result;                      /* hand the result back as void *  */
}

/* A "fake pthread_create": it does not create a thread, it just shows
 * how the library stores and later calls your function with your arg. */
static void *call_later(thread_fn fn, void *arg)
{
  return fn(arg);                     /* calling through the pointer     */
}

static void part5_function_pointers(void)
{
  long seven = 7;
  long *res = call_later(square, &seven);
  printf("5) square(7) via function pointer = %ld\n", *res);
  free(res);

  thread_fn table[] = {square, square};   /* arrays of function pointers */
  printf("   table[0] == square? %s\n", table[0] == square ? "yes" : "no");
}

/* ------------------------------------------------------------------ */
/* 6. Arrays and pointer arithmetic.  An array name "decays" to a      */
/*    pointer to its first element.  p + i moves by i *elements*,      */
/*    i.e. by i * sizeof(*p) bytes.                                    */
/* ------------------------------------------------------------------ */
static void part6_arrays(void)
{
  int a[4] = {10, 20, 30, 40};
  int *p = a;                         /* same as &a[0]                   */
  printf("6) a[2] = %d, *(p + 2) = %d, p[2] = %d\n", a[2], *(p + 2), p[2]);
  printf("   &a[1] - &a[0] = %td element(s) = %td bytes\n",
         &a[1] - &a[0], (char *)&a[1] - (char *)&a[0]);

  /* sizeof on the array itself gives the whole array, on the pointer
   * only the pointer: a classic bug when arrays are passed to functions. */
  printf("   sizeof(a) = %zu, sizeof(p) = %zu\n", sizeof(a), sizeof(p));
}

int main(void)
{
  part1_basics();
  part2_out_params();
  part3_pointer_to_pointer();
  part4_void_pointers();
  part5_function_pointers();
  part6_arrays();
  return 0;
}
