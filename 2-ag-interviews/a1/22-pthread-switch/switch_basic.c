#include <stdio.h>
#include <context.h>

context_t main_ctx, a_ctx, b_ctx;
char stack_a[16384] __attribute__((aligned(16)));
char stack_b[16384 + 8];          // deliberately NOT aligned: context_make must align it
char trace[16];
int pos = 0;
int local_kept = 0, on_own_stack = 0, aligned = 0;
long recursion_result = 0;

long sum(long n) { return n == 0 ? 0 : n + sum(n - 1); }   // uses the stack it runs on

void coroutineA(void* arg)
{
  int mine = (int) (size_t) arg;  // a local: must survive every switch
  for (int i = 0; i < 3; ++i)
  {
    trace[pos++] = 'a';
    context_switch(&a_ctx, &b_ctx);
  }
  local_kept = (mine == 7);
  context_switch(&a_ctx, &main_ctx);
}

void coroutineB(void* arg)
{
  char here;
  on_own_stack = (&here >= stack_b + 8 && &here < stack_b + sizeof(stack_b));
  char buffer[16] __attribute__((aligned(16)));
  aligned = ((size_t) buffer % 16 == 0);   // only true if rsp was aligned as the ABI requires
  recursion_result = sum(200);
  for (;;)
  {
    trace[pos++] = 'b';
    context_switch(&b_ctx, &a_ctx);
  }
}

int main()
{
  context_make(&a_ctx, stack_a, sizeof(stack_a), coroutineA, (void*) 7);
  context_make(&b_ctx, stack_b + 8, sizeof(stack_b) - 8, coroutineB, 0);
  int before = 1234;
  context_switch(&main_ctx, &a_ctx);       // returns when coroutineA switches back
  trace[pos] = 0;
  printf("[INFO] trace %s\n", trace);
  printf(trace[0] == 'a' && trace[1] == 'b' && trace[2] == 'a' && trace[3] == 'b' && trace[4] == 'a' &&
         trace[5] == 'b' && pos == 6 ? "[PASS] a and b take turns: ababab\n" : "[FAIL] wrong order\n");
  printf(before == 1234 ? "[PASS] main continues with its locals intact\n" : "[FAIL] main's locals lost\n");
  printf(local_kept ? "[PASS] a local of a coroutine survives its switches\n" : "[FAIL] coroutine local lost\n");
  printf(on_own_stack ? "[PASS] the coroutine runs on its own stack\n" : "[FAIL] not on the given stack\n");
  printf(aligned ? "[PASS] the stack is 16-byte aligned for the coroutine\n" : "[FAIL] misaligned stack\n");
  printf(recursion_result == 20100 ? "[PASS] deep recursion on the coroutine stack works\n"
                                   : "[FAIL] recursion on the coroutine stack\n");
  return 0;
}
