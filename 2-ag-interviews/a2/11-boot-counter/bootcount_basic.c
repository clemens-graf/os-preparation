#include <stdio.h>
#include <string.h>
#include <nonstd.h>

// run it in two boots on the same disk:  tools/run_test.sh -n 2 --keep-disk bootcount_basic.sweb
void makeMessage(char* out, size_t boot)     // "left by boot <n>" - SWEB's libc has no sprintf
{
  const char prefix[] = "left by boot ";
  memcpy(out, prefix, sizeof(prefix) - 1);
  char digits[20];
  int n = 0;
  do
  {
    digits[n++] = (char) ('0' + boot % 10);
    boot /= 10;
  } while (boot);
  for (int i = 0; i < n; ++i)
    out[sizeof(prefix) - 1 + i] = digits[n - 1 - i];
  out[sizeof(prefix) - 1 + n] = 0;
}

int main()
{
  size_t boot = bootcount();
  char motd[256];
  getmotd(motd);
  printf("[INFO] boot %zu, message of the day: '%s'\n", boot, motd);
  printf(boot >= 1 ? "[PASS] boot counter is running\n" : "[FAIL] no boot count\n");
  if (boot == 1)
    printf(motd[0] == 0 ? "[PASS] first boot: no message yet\n" : "[FAIL] message on a fresh disk\n");
  else
  {
    char expected[64];
    makeMessage(expected, boot - 1);
    printf(strcmp(motd, expected) == 0 ? "[PASS] the previous boot's message survived the reboot\n"
                                       : "[FAIL] message from the previous boot lost\n");
  }
  char mine[64];
  makeMessage(mine, boot);
  printf(setmotd(mine) == 0 ? "[PASS] message for the next boot written\n" : "[FAIL] setmotd failed\n");
  return 0;
}
