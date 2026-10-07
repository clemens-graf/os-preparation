#include <stdio.h>

// uses about 40 KiB of stack: 10 pages, only the topmost one is mapped at start
size_t sumBigArray()
{
  volatile char big[40 * 1024];
  for (size_t i = 0; i < sizeof(big); ++i)
    big[i] = (char) i;
  size_t sum = 0;
  for (size_t i = 0; i < sizeof(big); ++i)
    sum += (unsigned char) big[i];
  return sum;
}

int depth(int n)
{
  volatile char frame[512];
  frame[0] = (char) n;
  return n == 0 ? 0 : 1 + depth(n - 1) + frame[0] - (char) n;
}

int main()
{
  size_t expected = 0;
  for (size_t i = 0; i < 40 * 1024; ++i)
    expected += (unsigned char) (char) i;
  printf(sumBigArray() == expected ? "[PASS] 40 KiB stack array works\n" : "[FAIL] wrong sum\n");
  printf(depth(100) == 100 ? "[PASS] recursion depth 100 works\n" : "[FAIL] recursion broken\n");
  return 0;
}
