#include "rand.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

void binprintf16(uint16_t v) {
  uint16_t mask = 1 << (sizeof(uint16_t) * CHAR_BIT - 1);
  // printf("Generated random number: ");
  while (mask) {
    printf("%d", (v & mask ? 1 : 0));
    mask >>= 1;
  }
  printf("\n\n");
}

void binprintf32(uint32_t v) {
  uint32_t mask = 1 << (sizeof(uint32_t) * CHAR_BIT - 1);
  // printf("Generated random number: ");
  while (mask) {
    printf("%d", (v & mask ? 1 : 0));
    mask >>= 1;
  }
  printf("\n\n");
}

void binprintf64(unsigned long long v) {
  uint64_t mask = 1ULL << (sizeof(uint64_t) * CHAR_BIT - 1);
  // printf("Generated random number: ");
  while (mask) {
    printf("%d", (v & mask ? 1 : 0));
    mask >>= 1;
  }
  printf("\n\n");
}

#ifdef __rdrand__
#include <immintrin.h>

uint16_t get_rand16() {
  uint16_t rand;
  while (!_rdrand16_step(&rand))
    ;
  // printf("16-bit random number: %u\n", rand);
  binprintf16(rand);
  return rand;
}

uint32_t get_rand32() {
  uint32_t rand;
  while (!_rdrand32_step(&rand))
    ;
  // printf("32-bit random number: %u\n", rand);
  binprintf32(rand);
  return rand;
}

unsigned long long get_rand64() {
  unsigned long long rand;
  while (!_rdrand64_step(&rand))
    ;
  // printf("64-bit random number: %llu\n", rand);
  binprintf64(rand);
  return rand;
}

#else

#include <openssl/rand.h>

uint16_t get_rand16() {
  uint16_t random_number;
  if (RAND_bytes((unsigned char *)&random_number, sizeof(random_number)) != 1) {
    return 0;
  }
  binprintf16(random_number);
  return random_number;
}

uint32_t get_rand32() {
  uint32_t random_number;
  if (RAND_bytes((unsigned char *)&random_number, sizeof(random_number)) != 1) {
    return 0;
  }
  binprintf32(random_number);
  return random_number;
}

unsigned long long get_rand64() {
  unsigned long long random_number;
  if (RAND_bytes((unsigned char *)&random_number, sizeof(random_number)) != 1) {
    return 0;
  }
  binprintf64(random_number);
  return random_number;
}

#endif