#include <stdio.h>
#include <cpuid.h>
#include <stdbool.h>

bool checkRDRAND() {
    unsigned int eax, ebx, ecx, edx;
    
    // Call CPUID with function_id = 1
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        // Check bit 30 of ECX register
        return (ecx & (1 << 30)) != 0;
    }
    
    return false;
}

int main() {
    if (checkRDRAND()) {
        printf("RDRAND is supported\n");
    } else {
        printf("RDRAND is not supported\n");
    }
    return 0;
}
