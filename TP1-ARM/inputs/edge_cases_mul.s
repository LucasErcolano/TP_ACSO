.text
MOVZ X0, #0xFFFF, LSL #48       // Cargar el valor máximo en la parte alta
MOVZ X1, #0                     // Cargar un cero
MUL X2, XZR, X3                 // Multiplicación con cero

// Cargar el valor máximo en un registro y realizar la multiplicación
MOVZ X6, #0xFFFF, LSL #48
MOVK X6, #0xFFFF, LSL #32
MOVK X6, #0xFFFF, LSL #16
MOVK X6, #0xFFFF
MUL X4, X5, X6                  // Multiplicación con el valor máximo

HLT 0                           // Detener la ejecución
