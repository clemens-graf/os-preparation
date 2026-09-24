/*
 * once.c - Module 03 assignment, part C. The version below is the classic
 * broken check-then-act. Fix it.
 *
 * Bonus (not tested): after initialisation, every call still takes your
 * lock. Can you make the common "already done" path lock-free with an
 * atomic flag, without breaking the second guarantee? Search for
 * "double-checked locking" and think about acquire/release ordering.
 */
#include "once.h"

void my_once(my_once_t *o, void (*init)(void))
{
  if (!o->done) {
    init();
    o->done = 1;
  }
}
