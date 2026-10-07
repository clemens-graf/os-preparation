#include <stdio.h>
#include <nonstd.h>

int main()
{
  size_t ones = sum15(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
  printf(ones == 120 ? "[PASS] fifteen ones: 1 + 2 + ... + 15 = 120\n" : "[FAIL] wrong sum of ones\n");
  size_t only_last = sum15(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
  printf(only_last == 15 ? "[PASS] the 15th argument arrives as the 15th\n" : "[FAIL] 15th argument misplaced\n");
  size_t only_fifth = sum15(0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  printf(only_fifth == 5 ? "[PASS] the 5th argument (first one in memory) arrives as the 5th\n"
                         : "[FAIL] 5th argument misplaced\n");
  size_t big = sum15(1000000000000ULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  printf(big == 1000000000000ULL ? "[PASS] 64-bit values survive\n" : "[FAIL] 64-bit value truncated\n");
  return 0;
}
