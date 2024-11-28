#include <stdio.h>

int bar() {
  int a = 1;
  char s[3];
  int b = 2;

  fprintf(stdout, "Between a & s: %ld\n", (long)&a - (long)&s);
  fprintf(stdout, "Between s & b: %ld\n", (long)&s - (long)&b);

  printf("\nType some text: ");
  scanf("%s", s);

  fprintf(stdout, "\na: %d\ns: %s\nb: %d\n\n", a, s, b);

  return a + b;
}

int main() {
  int result = bar();
  fprintf(stdout, "%d\n", result);
  return 0;
}