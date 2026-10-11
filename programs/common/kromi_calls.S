/* KROM's two calls with their own registers: the diagnostic text (x0 the text, x1 its bytes, x2 stream 0) and the
 * program's end (x0 its status; it does not return).  Every other register a call does not use is zero.
 * Builds with the Kromi One SDK (ABI 2.0, revision 17). */

    .include "krom/abi2/layouts.inc"

    .section .text.kromi_write, "ax", %progbits
    .balign 16
    .global kromi_write
    .type kromi_write, %function
kromi_write:
    mov x2, xzr
    mov x3, xzr
    mov x4, xzr
    mov x5, xzr
    mov x6, xzr
    mov x7, xzr
    mov x8, #KROM_ABI2_SERVICE_DIAGNOSTIC_WRITE
    svc #KROM_ABI2_SVC_IMMEDIATE
    ret
    .size kromi_write, . - kromi_write

    .section .text.kromi_exit, "ax", %progbits
    .balign 16
    .global kromi_exit
    .type kromi_exit, %function
kromi_exit:
    mov w0, w0
    mov x1, xzr
    mov x2, xzr
    mov x3, xzr
    mov x4, xzr
    mov x5, xzr
    mov x6, xzr
    mov x7, xzr
    mov x8, #KROM_ABI2_SERVICE_EXIT
    svc #KROM_ABI2_SVC_IMMEDIATE
1:  brk #0
    b 1b
    .size kromi_exit, . - kromi_exit

    .section .note.GNU-stack, "", %progbits
