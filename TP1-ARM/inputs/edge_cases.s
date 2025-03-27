        .text
        .global main
main:
; ------------------------------
; Casos borde para ADDS (inmediato)
; ------------------------------
        ; immediate = 0
        adds_imm    X1, X2, #0
        ; immediate máximo sin shift (0xFFF = 4095)
        adds_imm    X3, X4, #4095
        ; immediate máximo con shift (shift=1, valor = 0xFFF << 12)
        adds_imm    X5, X6, #4095, LSL #12

; ------------------------------
; Casos borde para ADDS (registro)
; ------------------------------
        ; Extensión UXTB, sin shift
        adds_reg    X7, X8, X9, LSL #0, UXTB
        ; Extensión SXTW, shift máximo (imm3 = 7)
        adds_reg    X10, X11, X12, LSL #7, SXTW

; ------------------------------
; Casos borde para SUBS (inmediato)
; ------------------------------
        subs_imm    X13, X14, #0
        subs_imm    X15, X16, #4095
        subs_imm    X17, X18, #4095, LSL #12

; ------------------------------
; Casos borde para SUBS (registro)
; ------------------------------
        subs_reg    X19, X20, X21, LSL #0, UXTH
        subs_reg    X22, X23, X24, LSL #7, SXTW

; ------------------------------
; Casos borde para ANDS
; ------------------------------
        ands        X25, X26, X27, LSL #0      ; sin shift
        ands        X28, X29, X30, LSL #63     ; máximo shift (inmediato = 63)
        ands        X1, X2, X3, LSR #31       ; shift derecho extremo

; ------------------------------
; Casos borde para EOR
; ------------------------------
        eor         X4, X5, X6, LSL #0
        eor         X7, X8, X9, LSR #31

; ------------------------------
; Caso para ORR
; ------------------------------
        orr         X10, X11, X12

; ------------------------------
; Casos para ramas incondicionales y de registro
; ------------------------------
        ; Rama incondicional: saltar hacia adelante
        b           branch_forward
nop:
        nop
branch_forward:
        nop
        ; Rama incondicional: saltar hacia atrás
        b           branch_backward
branch_backward:
        nop

        ; Rama por registro: mover dirección y saltar
        mov         X1, branch_forward
        br          X1

; ------------------------------
; Casos para rama condicional (B.cond)
; ------------------------------
        b_cond      BEQ, label_BEQ     ; salta si FLAG_Z == 1
        b_cond      BNE, label_BNE     ; salta si FLAG_Z == 0
        b_cond      BGE, label_BGE     ; salta si FLAG_N == 0
        b_cond      BLT, label_BLT     ; salta si FLAG_N != 0
        b_cond      BGT, label_BGT     ; salta si FLAG_Z == 0 y FLAG_N == 0
        b_cond      BLE, label_BLE     ; salta si FLAG_Z == 1 o FLAG_N != 0
label_BEQ:
        nop
label_BNE:
        nop
label_BGE:
        nop
label_BLT:
        nop
label_BGT:
        nop
label_BLE:
        nop

; ------------------------------
; Casos para CBZ y CBNZ
; ------------------------------
        ; CBZ: debe saltar si el registro es 0
        mov         X1, #0
        cbz         X1, label_cbz
        ; CBZ: no salta si el registro es distinto de 0
        mov         X2, #1
        cbz         X2, label_cbz_fail
label_cbz:
        nop
label_cbz_fail:
        nop

        ; CBNZ: debe saltar si el registro es distinto de 0
        mov         X3, #1
        cbnz        X3, label_cbnz
        ; CBNZ: no salta si el registro es 0
        mov         X4, #0
        cbnz        X4, label_cbnz_fail
label_cbnz:
        nop
label_cbnz_fail:
        nop

; ------------------------------
; Casos para LDUR y STUR (64 bits)
; ------------------------------
        ; Almacenamiento en dirección alineada
        mov         X6, #0x1000
        stur        X5, [X6, #0]
        ; Almacenamiento en dirección no alineada (debe emitir advertencia)
        mov         X8, #0x1003
        stur        X7, [X8, #7]
        ldur        X9, [X10, #0]
        ldur        X11, [X12, #7]

; ------------------------------
; Caso para MOVZ
; ------------------------------
        movz        X13, #1234            ; caso válido (hw = 0 implícito)
        ; Caso borde: hw distinto de 0 (se espera mensaje de advertencia)
        movz_bad    X14, #5678

; ------------------------------
; Caso para MUL
; ------------------------------
        mul         X15, X16, X17

; ------------------------------
; Casos para SHIFTS (LSL y LSR)
; ------------------------------
        shifts      X18, X19, #0           ; sin desplazamiento
        shifts      X20, X21, #63          ; máximo desplazamiento

; ------------------------------
; Casos para STURB y LDURB (8 bits)
; ------------------------------
        sturb       X22, [X23, #0]
        ldurb       X24, [X25, #0]

; ------------------------------
; Casos para STURH y LDURH (16 bits)
; ------------------------------
        ; Uso de dirección no alineada (debe emitir advertencia)
        sturh       X26, [X27, #1]
        ldurh       X28, [X29, #1]

; ------------------------------
; Casos para ADD (inmediato y registro, sin actualizar flags)
; ------------------------------
        add_imm     X30, X31, #0
        add_imm     X1, X2, #4095, LSL #12
        add_reg     X3, X4, X5, LSL #0, UXTX
        add_reg     X6, X7, X8, LSL #7, SXTW

; ------------------------------
; Final: detener la simulación
; ------------------------------
        hlt
