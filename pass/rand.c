#include <immintrin.h>
#include <stdint.h>

void get_rand(uint16_t *rand) {
    int status;
    while(!_rdrand16_step(rand));
}
