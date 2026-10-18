#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int compareInts(const void *a, const void *b)
{
  int x = *(const int *) a, y = *(const int *) b;
  return (x > y) - (x < y);           // not x - y: that can overflow
}

typedef struct
{
  char name[8];
  int age;
} Person;

int compareByAge(const void *a, const void *b)
{
  return ((const Person *) a)->age - ((const Person *) b)->age;
}

int main()
{
  int numbers[] = { 5, -3, 9, 0, 9, 2147483647, -2147483647 - 1, 1 };
  size_t n = sizeof(numbers) / sizeof(numbers[0]);
  qsort(numbers, n, sizeof(int), compareInts);
  int sorted = 1;
  for (size_t i = 1; i < n; ++i)
    sorted &= numbers[i - 1] <= numbers[i];
  printf(sorted ? "[PASS] qsort sorts ints (also INT_MIN / INT_MAX)\n" : "[FAIL] ints not sorted\n");

  Person people[] = { { "carol", 41 }, { "alice", 23 }, { "bob", 35 } };
  qsort(people, 3, sizeof(Person), compareByAge);
  printf(strcmp(people[0].name, "alice") == 0 && strcmp(people[2].name, "carol") == 0
             ? "[PASS] qsort sorts structs (element size 12)\n" : "[FAIL] structs not sorted\n");

  qsort(numbers, 0, sizeof(int), compareInts);
  printf("[PASS] qsort with 0 elements did not crash\n");

  char line[] = "  ls -l  /usr ";
  char *a = strtok(line, " ");
  char *b = strtok(0, " ");
  char *c = strtok(0, " ");
  char *d = strtok(0, " ");
  printf(a && b && c && !d && strcmp(a, "ls") == 0 && strcmp(b, "-l") == 0 && strcmp(c, "/usr") == 0
             ? "[PASS] strtok splits a command line\n" : "[FAIL] strtok tokens wrong\n");
  printf(strtok(0, " ") == 0 ? "[PASS] strtok keeps returning NULL at the end\n"
                             : "[FAIL] strtok continued after the end\n");

  char path[] = "a/b//c";
  int count = 0;
  for (char *t = strtok(path, "/"); t; t = strtok(0, "/"))
    ++count;
  printf(count == 3 ? "[PASS] strtok skips empty tokens\n" : "[FAIL] wrong token count\n");

  char spaces[] = "   ";
  printf(strtok(spaces, " ") == 0 ? "[PASS] only delimiters: NULL\n" : "[FAIL] token in an empty string\n");
  return 0;
}
