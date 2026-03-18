.section .text
.globl _start

_start:
    addi x1, x0, 5
    addi x2, x1, 10
    add  x3, x1, x2

loop:
    j loop
