#include <stdio.h>
#include <nonstd.h>

int global_variable = 1;

int main()
{
  struct segment seg[8];
  int n = segments(seg, 8);
  printf("[INFO] %d segments\n", n);
  int code_found = 0;
  int data_found = 0;
  for (int i = 0; i < n; ++i)
  {
    printf("[INFO] %lx - %lx  file %lx  %c%c%c\n", seg[i].vaddr, seg[i].vaddr + seg[i].memsz, seg[i].filesz,
           seg[i].flags & 4 ? 'r' : '-', seg[i].flags & 2 ? 'w' : '-', seg[i].flags & 1 ? 'x' : '-');
    size_t main_address = (size_t) main;
    size_t data_address = (size_t) &global_variable;
    if (main_address >= seg[i].vaddr && main_address < seg[i].vaddr + seg[i].memsz && (seg[i].flags & 1))
      code_found = 1;
    if (data_address >= seg[i].vaddr && data_address < seg[i].vaddr + seg[i].memsz && (seg[i].flags & 2))
      data_found = 1;
  }
  printf(code_found ? "[PASS] main is in an executable segment\n" : "[FAIL] main not found\n");
  printf(data_found ? "[PASS] global_variable is in a writable segment\n" : "[FAIL] data not found\n");
  printf(segments(seg, 0) == -1 ? "[PASS] max 0 rejected\n" : "[FAIL] max 0 accepted\n");
  return 0;
}
