#include <stdint.h>

// Helper to wrap the assembly
uint32_t test_mulh(int32_t a, int32_t b) {
    uint32_t res;
    __asm__ __volatile__ (
        "mulh %0, %1, %2" 
        : "=r"(res)  // Output: write to 'res'
        : "r"(a), "r"(b) // Inputs: read from 'a' and 'b'
    );
    return res;
}

uint32_t test_mulhu(uint32_t a, uint32_t b) {
    uint32_t res;
    __asm__ __volatile__ (
        "mulhu %0, %1, %2" 
        : "=r"(res) 
        : "r"(a), "r"(b)
    );
    return res;
}

int main() {
    uint32_t val = 0xFFFFFFFF; // -1 signed, max unsigned
    
    // Test 1: MULH (Signed)
    // -1 * -1 = +1. The 64-bit result is 0x00000000 00000001.
    // The High bits (MULH) should be 0.
    uint32_t high_signed = test_mulh((int32_t)val, (int32_t)val);

    // Test 2: MULHU (Unsigned)
    // (2^32-1) * (2^32-1) = 2^64 - 2^33 + 1
    // Hex: 0xFFFFFFFE 00000001
    // The High bits (MULHU) should be 0xFFFFFFFE.
    uint32_t high_unsigned = test_mulhu(val, val);

    // If both passed, return a unique success code (e.g., 42)
    if (high_signed == 0 && high_unsigned == 0xFFFFFFFE) {
        return 42;
    }

    return 1; // Failure
}