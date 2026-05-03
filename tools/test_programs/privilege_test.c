#define UART_ADDR 0x10000000
#define CLINT_MTIMECMP 0x02004000
#define CLINT_MTIME    0x0200BFF8

void __attribute__((aligned(4))) trap_handler() {
    char *msg = "INTERRUPT TAKEN\n";
    while (*msg) *(volatile char*)UART_ADDR = *msg++;
    // Clear the interrupt by pushing mtimecmp far into the future
    *(volatile unsigned long long*)CLINT_MTIMECMP = 0xFFFFFFFFFFFFFFFFULL;
}

int main() {
    // 1. Set mtvec
    asm volatile("csrw mtvec, %0" : : "r"(trap_handler));
    
    // 2. Set timer for 1000 ticks from now
    unsigned long long now = *(volatile unsigned long long*)CLINT_MTIME;
    *(volatile unsigned long long*)CLINT_MTIMECMP = now + 1000;

    // 3. Enable interrupts
    asm volatile("csrsi mstatus, 8"); // MIE bit (8 is within 5-bit range)

    // Fix: Use a register for 128 (MTIE bit)
    int mtie_bit = 128;
    asm volatile("csrs mie, %0" : : "r"(mtie_bit));

    while(1) { asm volatile("wfi"); }
    return 0;
}
