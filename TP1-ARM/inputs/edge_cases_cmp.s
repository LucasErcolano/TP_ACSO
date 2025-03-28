.text
CMP X0, X0                      // Comparar un registro consigo mismo
CMP X1, #0                      // Comparar un registro con cero

// Cargar el valor máximo en un registro y compararlo
MOVZ X4, #0xFFFF, LSL #48
MOVK X4, #0xFFFF, LSL #32
MOVK X4, #0xFFFF, LSL #16
MOVK X4, #0xFFFF
CMP X2, X4                      // Comparar con el valor máximo

// Cargar el valor mínimo en un registro y compararlo
MOVZ X5, #0x8000, LSL #48
CMP X3, X5                      // Comparar con el valor mínimo

HLT 0                           // Detener la ejecución
