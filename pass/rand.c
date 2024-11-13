#include <stdint.h>
#include "rand.h"

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