#include <immintrin.h>
#include <stdint.h>
#include "rand.h"

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