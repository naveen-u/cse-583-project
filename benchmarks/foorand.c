#include <stdio.h>
#include <stdint.h>

int32_t get_urand(){
    int32_t rand_num = 0;
    int ok = 0;
    uint32_t *rand = (uint32_t*)&rand_num;
    while(!ok) {
        unsigned char ok;
        asm volatile ("rdrand %0; setc %1"
            : "=r" (*rand), "=qm" (ok));
    }
    return rand_num & 0b1111;
}

int main() {
    int a = 1;
    int b = 2;

    fprintf(stdout, "%d\n", a + b);

    return 0;
}