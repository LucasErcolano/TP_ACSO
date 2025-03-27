    .section .text
    .global _start

_start:
    @ ----------------------------
    @ ADDS Immediate Tests:
    @ ----------------------------
    ADDS X0, X1, #0            @ X0 = X1 + 0; update flags
    @ Immediate máximo sin shift (0xFFF = 4095)
    ADDS X2, X3, #4095         @ X2 = X3 + 4095; update flags
    @ Immediate máximo con shift (shift=1, valor=0xFFF<<12)
    ADDS X4, X5, #4095, LSL #12 @ X4 = X5 + (4095 << 12); update flags

    @ ----------------------------
    @ ADDS Register Tests (Extended Register variant)
    @ ----------------------------
    ADDS X6, X7, X8            @ X6 = X7 + X8; update flags
    @ (Extended register with extension such as UXTB is not directly supported here)
    ADDS X9, X10, X11           @ X9 = X10 + X11; update flags

    @ ----------------------------
    @ SUBS Immediate Tests:
    @ ----------------------------
    SUBS X12, X13, #0          @ X12 = X13 - 0; update flags
    SUBS X14, X15, #4095        @ X14 = X15 - 4095; update flags
    SUBS X16, X17, #4095, LSL #12 @ X16 = X17 - (4095 << 12); update flags

    @ ----------------------------
    @ SUBS Register Tests:
    @ ----------------------------
    SUBS X18, X19, X20         @ X18 = X19 - X20; update flags
    SUBS X21, X22, X23         @ X21 = X22 - X23; update flags

    @ ----------------------------
    @ CMP Tests:
    @ ----------------------------
    CMP X24, X25               @ Compare X24 and X25; update flags
    CMP X26, #4                @ Compare X26 with immediate 4; update flags

    @ ----------------------------
    @ Logical Operations:
    @ ----------------------------
    ANDS X27, X28, X29         @ X27 = X28 AND X29; update flags
    EOR X30, X1, X2            @ X30 = X1 XOR X2
    ORR X2, X3, X4             @ X2 = X3 OR X4

    @ ----------------------------
    @ Branch Instructions:
    @ ----------------------------
    B label_forward            @ Unconditional branch to label_forward
    NOP                        @ This NOP is skipped if branch works correctly

label_forward:
    @ Branch via register:
    BR X0                      @ Branch to address contained in X0

    @ Conditional Branches (based on previous CMP; flags assumed to be set)
    BEQ label_beq              @ Branch if equal
    BNE label_bne              @ Branch if not equal
    BGT label_bgt              @ Branch if greater than
    BLT label_blt              @ Branch if less than
    BGE label_bge              @ Branch if greater than or equal
    BLE label_ble              @ Branch if less than or equal

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
    @ ----------------------------
    @ CBZ / CBNZ Tests:
    @ ----------------------------
    CBZ X3, label_cbz         @ Branch if X3 is zero
    CBNZ X4, label_cbnz       @ Branch if X4 is not zero

label_cbz:
    NOP
    B next_section
label_cbnz:
    NOP

next_section:
    @ ----------------------------
    @ Memory (Load/Store) Tests:
    @ ----------------------------
    STUR X5, [X6, #16]         @ Store 64-bit value from X5 at address (X6 + 16)
    LDUR X7, [X8, #16]         @ Load 64-bit value into X7 from address (X8 + 16)

    @ STURB and LDURB (8-bit operations)
    STURB W9, [X10, #16]       @ Store lower 8 bits of W9 at address (X10 + 16)
    LDURB W11, [X12, #16]       @ Load 8 bits into W11 from address (X12 + 16)

    @ STURH and LDURH (16-bit operations)
    STURH W13, [X14, #16]       @ Store lower 16 bits of W13 at address (X14 + 16)
    LDURH W15, [X16, #16]       @ Load 16 bits into W15 from address (X16 + 16)

    @ ----------------------------
    @ MOVZ Test:
    @ ----------------------------
    MOVZ X17, #10              @ Move immediate 10 into X17

    @ ----------------------------
    @ ADD (without updating flags) Tests:
    @ ----------------------------
    ADD X18, X19, #3           @ X18 = X19 + 3 (immediate)
    ADD X20, X21, X22          @ X20 = X21 + X22 (register)

    @ ----------------------------
    @ MUL Test:
    @ ----------------------------
    MUL X23, X24, X25          @ X23 = X24 * X25

    @ ----------------------------
    @ Halt Simulation:
    @ ----------------------------
    HLT #0                   @ Halt (set RUN_BIT to 0 in simulator)

    @ End of Program
