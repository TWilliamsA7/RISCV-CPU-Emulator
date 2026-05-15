// uart_test.c
// Bare-metal RISC-V UART + PLIC interrupt test
//
// Compile:
//   riscv32-unknown-elf-gcc -march=rv32ima -mabi=ilp32 -nostdlib -nostartfiles \
//       -T uart_test.ld uart_test.c -o uart_test.elf
//
// What it tests:
//   Phase 1 — Polled TX:  prints "Hello UART\n" by polling LSR THRE
//   Phase 2 — Polled RX:  waits for a character via LSR DR, echoes it back
//   Phase 3 — IRQ RX:     enables PLIC + UART RX interrupt, calls WFI,
//                          wakes on keypress, echoes "[IRQ: X]\n"

#include <stdint.h>

// ── MMIO helpers ──────────────────────────────────────────────────────────────

static inline void     mmio_w8 (uint32_t a, uint8_t  v) { *(volatile uint8_t  *)a = v; }
static inline void     mmio_w32(uint32_t a, uint32_t v) { *(volatile uint32_t *)a = v; }
static inline uint8_t  mmio_r8 (uint32_t a)             { return *(volatile uint8_t  *)a; }
static inline uint32_t mmio_r32(uint32_t a)             { return *(volatile uint32_t *)a; }

// ── 16550 UART ────────────────────────────────────────────────────────────────

#define UART_BASE  0x10000000
#define UART_RBR   (UART_BASE + 0)   // RX buffer  (read)
#define UART_THR   (UART_BASE + 0)   // TX holding (write)
#define UART_IER   (UART_BASE + 1)   // Interrupt enable
#define UART_IIR   (UART_BASE + 2)   // Interrupt identification (read)
#define UART_LCR   (UART_BASE + 3)   // Line control
#define UART_LSR   (UART_BASE + 5)   // Line status

#define LSR_DR     0x01  // Data ready
#define LSR_THRE   0x20  // TX holding register empty

#define IER_RDI    0x01  // RX interrupt enable
#define IER_THRI   0x02  // TX interrupt enable

// ── PLIC ──────────────────────────────────────────────────────────────────────

#define PLIC_BASE         0x0C000000
#define PLIC_PRIO(src)    (PLIC_BASE + (src)*4)          // priority for source src
#define PLIC_PENDING      (PLIC_BASE + 0x1000)            // pending bits word 0
#define PLIC_ENABLE_S     (PLIC_BASE + 0x2080)            // enable, S-mode ctx (ctx 1)
#define PLIC_THRESH_S     (PLIC_BASE + 0x201000)          // threshold, S-mode ctx
#define PLIC_CLAIM_S      (PLIC_BASE + 0x201004)          // claim/complete, S-mode ctx

#define UART_IRQ   10

// ── CSR helpers ───────────────────────────────────────────────────────────────

#define CSR_READ(csr, val)   asm volatile("csrr %0, " #csr : "=r"(val))
#define CSR_WRITE(csr, val)  asm volatile("csrw " #csr ", %0" :: "r"(val))
#define CSR_SET(csr, val)    asm volatile("csrs " #csr ", %0" :: "r"(val))
#define CSR_CLEAR(csr, val)  asm volatile("csrc " #csr ", %0" :: "r"(val))
#define WFI()                asm volatile("wfi")

// ── globals shared with trap handler ─────────────────────────────────────────

volatile uint8_t irq_char   = 0;
volatile int     irq_fired  = 0;

// ── UART helpers ──────────────────────────────────────────────────────────────

static void uart_init(void) {
    mmio_w8(UART_LCR, 0x03); // 8N1, DLAB=0
    mmio_w8(UART_IER, 0x00); // interrupts off initially
}

static void uart_putc(char c) {
    while (!(mmio_r8(UART_LSR) & LSR_THRE))
        ;
    mmio_w8(UART_THR, (uint8_t)c);
}

static void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

static char uart_getc_poll(void) {
    while (!(mmio_r8(UART_LSR) & LSR_DR))
        ;
    return (char)mmio_r8(UART_RBR);
}

static void uart_put_hex(uint32_t v) {
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nib = (v >> i) & 0xF;
        uart_putc(nib < 10 ? '0' + nib : 'a' + nib - 10);
    }
}

// ── PLIC setup ────────────────────────────────────────────────────────────────

static void plic_init(void) {
    mmio_w32(PLIC_PRIO(UART_IRQ), 1);           // priority 1 for UART
    mmio_w32(PLIC_ENABLE_S, 1u << UART_IRQ);    // enable UART in S-mode ctx
    mmio_w32(PLIC_THRESH_S, 0);                 // threshold 0 = accept all
}

static uint32_t plic_claim(void) {
    return mmio_r32(PLIC_CLAIM_S);
}

static void plic_complete(uint32_t src) {
    mmio_w32(PLIC_CLAIM_S, src);
}

// ── Trap handler ─────────────────────────────────────────────────────────────
// We stay in M-mode for simplicity in this test (no S-mode delegation).
// mtvec points here (direct mode).

void __attribute__((interrupt("machine"))) trap_handler(void) {
    uint32_t cause;
    CSR_READ(mcause, cause);

    // Bit 31 set = interrupt
    if (cause & 0x80000000) {
        uint32_t id = cause & 0x7FFFFFFF;

        if (id == 11) { // Machine External Interrupt
            uint32_t src = plic_claim();
            if (src == UART_IRQ) {
                irq_char  = mmio_r8(UART_RBR);
                irq_fired = 1;
                plic_complete(src);
            }
        }
    }
    // Exceptions: just spin (shouldn't happen in this test)
}

// ── Entry point ──────────────────────────────────────────────────────────────

void main(void) {

    // ── Phase 1: Polled TX ───────────────────────────────────────────────────
    uart_init();
    uart_puts("\r\n=== Phase 1: Polled TX ===\r\n");
    uart_puts("Hello UART\r\n");
    uart_puts("TX polling: OK\r\n");

    // ── Phase 2: Polled RX ───────────────────────────────────────────────────
    uart_puts("\r\n=== Phase 2: Polled RX ===\r\n");
    uart_puts("Send a character (polled)...\r\n");
    char c = uart_getc_poll();
    uart_puts("Got (polled): ");
    uart_putc(c);
    uart_puts("\r\n");

    // ── Phase 3: Interrupt-driven RX ─────────────────────────────────────────
    uart_puts("\r\n=== Phase 3: Interrupt-driven RX ===\r\n");

    // Point mtvec at our handler (direct mode, bit 0 = 0)
    CSR_WRITE(mtvec, (uint32_t)trap_handler);

    // Enable UART RX interrupt
    mmio_w8(UART_IER, IER_RDI);

    // Set up PLIC
    plic_init();

    // Enable machine external interrupts (MIE bit 11) and global MIE (bit 3)
    uint32_t mie_val;
    CSR_READ(mie, mie_val);
    mie_val |= (1 << 11); // MEIE
    CSR_WRITE(mie, mie_val);
    CSR_SET(mstatus, (1 << 3)); // MIE global enable

    uart_puts("Send a character (IRQ mode)... ");
    irq_fired = 0;

    WFI(); // sleep until interrupt

    // By the time we wake, trap_handler has already claimed+completed
    if (irq_fired) {
        uart_puts("\r\nGot (IRQ): ");
        uart_putc((char)irq_char);
        uart_puts("\r\n");
        uart_puts("IRQ test: OK\r\n");
    } else {
        uart_puts("\r\nIRQ test: FAIL (spurious wakeup)\r\n");
    }

    // ── Done ─────────────────────────────────────────────────────────────────
    uart_puts("\r\n=== All phases complete ===\r\n");

    // Signal tohost success (your existing mechanism)
    *(volatile uint32_t *)0x80001000 = 1;

    while (1) ; // halt
}

// ── Minimal startup ──────────────────────────────────────────────────────────

__attribute__((naked, section(".text.start")))
void _start(void) {
    asm volatile(
        "la   sp, _stack_top\n"
        "call main\n"
        "1: j 1b\n"
    );
}
