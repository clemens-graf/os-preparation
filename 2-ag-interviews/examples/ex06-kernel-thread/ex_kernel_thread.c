#include <stdio.h>
#include <nonstd.h>

int main()
{
  printf(ticker(0) == -1 ? "[PASS] count 0 rejected\n" : "[FAIL] count 0 accepted\n");
  printf("[INFO] starting the ticker - watch for 'tick' lines\n");
  printf(ticker(3) == 3 ? "[PASS] ticker finished\n" : "[FAIL] ticker failed\n");
  printf(ticker(2) == 2 ? "[PASS] second ticker finished\n" : "[FAIL] second ticker failed\n");
  return 0;
}
