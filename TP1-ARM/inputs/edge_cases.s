    .section .text
    .global _start

_start:
    ADDS X0, X1, #0
    ADDS X2, X3, #4095
    ADDS X4, X5, #4095, LSL #12
    ADDS X6, X7, X8
    ADDS X9, X10, X11
    SUBS X12, X13, #0
    SUBS X14, X15, #4095
    SUBS X16, X17, #4095, LSL #12
    SUBS X18, X19, X20
    SUBS X21, X22, X23
    CMP X24, X25
    CMP X26, #4
    ANDS X27, X28, X29
    EOR X30, X1, X2
    ORR X2, X3, X4
    B label_forward
    NOP

label_forward:
    BR X0
    BEQ label_beq
    BNE label_bne
    BGT label_bgt
    BLT label_blt
    BGE label_bge
    BLE label_ble

label_beq:
    NOP
    B end_conditions
label_bne:
    NOP
    B end_conditions
label_bgt:
    NOP
    B end_conditions
label_blt:
    NOP
    B end_conditions
label_bge:
    NOP
    B end_conditions
label_ble:
    NOP

end_conditions:
    CBZ X3, label_cbz
    CBNZ X4, label_cbnz

label_cbz:
    NOP
    B next_section
label_cbnz:
    NOP

next_section:
    STUR X5, [X6, #16]
    LDUR X7, [X8, #16]
    STURB W9, [X10, #16]
    LDURB W11, [X12, #16]
    STURH W13, [X14, #16]
    LDURH W15, [X16, #16]
    MOVZ X17, #10
    ADD X18, X19, #3
    ADD X20, X21, X22
    MUL X23, X24, X25

    HLT 0