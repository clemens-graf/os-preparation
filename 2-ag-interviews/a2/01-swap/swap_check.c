#include <stdio.h>
#include <nonstd.h>

int main()
{
  printf(swapinfo() == 0 ? "[PASS] no swap slots left in use\n" : "[FAIL] swap slots leaked\n");
  return 0;
}
