#include <stdio.h>
#include <nonstd.h>

size_t square(size_t x)
{
  return x * x;
}

size_t chatty(size_t x)
{
  printf("[INFO] chatty(%zu) runs inside the upcall\n", x);
  return x + 1;
}

size_t nested(size_t x)
{
  return upcall(square, x) + 1;     // an upcall from inside an upcall
}

int main()
{
  size_t a = 11, b = 22, c = 33;  // locals of main must survive
  printf(upcall(square, 7) == 49 ? "[PASS] upcall(square, 7) == 49\n" : "[FAIL] wrong result\n");
  printf(upcall(chatty, 41) == 42 ? "[PASS] function with printf works\n" : "[FAIL] chatty broken\n");
  printf(upcall(nested, 5) == 26 ? "[PASS] nested upcall works\n" : "[FAIL] nested upcall broken\n");
  size_t sum = 0;
  for (size_t i = 0; i < 100; ++i)
    sum += upcall(square, i);
  printf(sum == 328350 ? "[PASS] 100 upcalls in a loop\n" : "[FAIL] loop sum wrong\n");
  printf(a == 11 && b == 22 && c == 33 ? "[PASS] caller's locals intact\n" : "[FAIL] caller's locals clobbered\n");
  printf(upcall((size_t (*)(size_t)) 0xFFFFFFFF80000000ULL, 1) == (size_t) -1 ? "[PASS] kernel address rejected\n"
                                                                             : "[FAIL] kernel address accepted\n");
  return 0;
}
