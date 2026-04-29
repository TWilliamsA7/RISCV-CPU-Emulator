.section .text.entry
.globl _start

_start:
    # 1. Set up the stack pointer. 
    # Our RAM ends at 0x88000000 (if 128MB starting at 0x80000000)
    # The stack grows downward.
    li sp, 0x88000000

    # 2. Jump to C 'main' function
    call main

    # a7 = 93 (SYS_exit), a0 = return value (already in a0 from main)
    li a7, 93
    ecall
    