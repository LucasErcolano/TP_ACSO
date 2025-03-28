.text
ANDS X0, XZR, XZR               // AND lógico entre ceros
EOR X1, XZR, XZR                // XOR lógico entre ceros
ORR X2, XZR, XZR                // OR lógico entre ceros

// Cargar todos los bits en 1 en un registro y realizar AND lógico
MOVZ X3, #0xFFFF, LSL #48
MOVK X3, #0xFFFF, LSL #32
MOVK X3, #0xFFFF, LSL #16
MOVK X3, #0xFFFF
ANDS X4, XZR, X3                // AND lógico con todos los bits en 1

// Cargar el bit más significativo en un registro y realizar AND lógico
MOVZ X5, #0x8000, LSL #48
ANDS X6, X5, X5                 // AND lógico con el bit más significativo

// Cargar todos los bits en 1 en un registro y realizar XOR lógico
MOVZ X7, #0xFFFF, LSL #48
MOVK X7, #0xFFFF, LSL #32
MOVK X7, #0xFFFF, LSL #16
MOVK X7, #0xFFFF
EOR X8, XZR, X7                 // XOR lógico con todos los bits en 1

// Cargar el bit más significativo en un registro y realizar OR lógico
MOVZ X9, #0x8000, LSL #48
ORR X10, XZR, X9                // OR lógico con el bit más significativo

HLT 0                           // Detener la ejecución
