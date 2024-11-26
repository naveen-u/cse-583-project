#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include "rand.h"

void binprintf(uint32_t v)
{
    unsigned int mask = 1 << (sizeof(uint32_t) * CHAR_BIT  - 1);
    printf("Generated random number: ");
    while(mask) {
        printf("%d", (v&mask ? 1 : 0));
        mask >>= 1;
    }
    printf("\n\n");
}

#ifdef __rdrand__
#include <immintrin.h>

uint16_t get_rand16() {
    uint16_t rand;
    while(!_rdrand16_step(&rand));
    return rand;
}

uint32_t get_rand32() {
    uint32_t rand;
    while(!_rdrand32_step(&rand));
    binprintf(rand);
    return rand;
}

unsigned long long get_rand64() {
    unsigned long long rand;
    while(!_rdrand64_step(&rand));
    return rand;
}

#else

#include <openssl/rand.h>

uint16_t get_rand16() {
    uint16_t random_number;
    if (RAND_bytes((unsigned char*)&random_number, sizeof(random_number)) != 1) {
        return 0;
    }
    return random_number;
}

uint32_t get_rand32() {
    uint32_t random_number;
    if (RAND_bytes((unsigned char*)&random_number, sizeof(random_number)) != 1) {
        return 0;
    }
    binprintf(random_number);
    return random_number;
}

unsigned long long get_rand64() {
    unsigned long long random_number;
    if (RAND_bytes((unsigned char*)&random_number, sizeof(random_number)) != 1) {
        return 0;
    }
    return random_number;
}

#endif