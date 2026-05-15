/* uart_test.ld — bare-metal linker script for RISC-V emulator */

ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS {
    . = 0x80000000;

    .text : {
        *(.text.start)   /* _start first */
        *(.text*)
        *(.rodata*)
    } > RAM

    .data : {
        *(.data*)
    } > RAM

    .bss : {
        *(.bss*)
        *(COMMON)
    } > RAM

    /* 16KB stack at top of a reserved area */
    . = ALIGN(16);
    _stack_bottom = .;
    . += 0x4000;
    _stack_top = .;
}
