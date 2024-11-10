#include <stdio.h>

int* bar() {
    int a = 1;
    int b = 2;

    fprintf(stdout, "%ld\n", (long)&a);
    fprintf(stdout, "%ld\n", (long)&b);
    fprintf(stdout, "%ld\n", (long)&a - (long)&b);
    int c = a + b;
    return &c;
}

int main() {
    int* r = bar();
    fprintf(stdout, "%ld\n", (long)r);
    return 0;
}