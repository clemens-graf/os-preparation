#include <stdio.h>
#include <context.h>

context_t main_ctx, c_ctx;
char stack_c[8192] __attribute__((aligned(16)));

void finishes(void* arg)
{
  printf("[PASS] the coroutine ran - it returns now, which ends the process\n");
}

int main()
{
  context_make(&c_ctx, stack_c, sizeof(stack_c), finishes, 0);
  context_switch(&main_ctx, &c_ctx);
  printf("[FAIL] main continued after the coroutine returned\n");
  return 0;
}
