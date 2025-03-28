.text
// Cargar valores extremos en registros
MOVZ X4, #0xFFFF, LSL #48
MOVK X4, #0xFFFF, LSL #32
MOVK X4, #0xFFFF, LSL #16
MOVK X4, #0xFFFF

MOVZ X5, #0x8000, LSL #48

// Casos borde
ADDS X0, X1, X4                // Desbordamiento positivo
SUBS X2, X3, #1                // Desbordamiento negativo
ADDS X0, XZR, XZR              // Suma de ceros
SUBS X1, XZR, XZR              // Resta de ceros
ADDS X2, XZR, X4               // Suma con el valor máximo
SUBS X3, XZR, X5               // Resta con el valor mínimo

HLT 0