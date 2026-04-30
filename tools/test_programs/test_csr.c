int main() {
    // 1. Test CSRRW (Write 0x5 to mscratch, read old value)
    unsigned int old_val;
    unsigned int test_val = 0x5;
    asm volatile ("csrrw %0, mscratch, %1" : "=r"(old_val) : "r"(test_val));

    // 2. Test CSRRS (Set bit 3 of mscratch -> should become 0xD)
    unsigned int set_mask = 0x8;
    asm volatile ("csrrs x0, mscratch, %0" : : "r"(set_mask));

    // 3. Test CSRRC (Clear bit 0 of mscratch -> should become 0xC)
    unsigned int clear_mask = 0x1;
    asm volatile ("csrrc x0, mscratch, %0" : : "r"(clear_mask));

    // 4. Read back final value
    unsigned int final_val;
    asm volatile ("csrr %0, mscratch" : "=r"(final_val));

    return final_val; // Should exit with 12 (0xC)
}