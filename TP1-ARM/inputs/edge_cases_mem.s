.text
// Direcciones no alineadas
STUR X0, [X1, #3]               // Dirección no alineada
LDUR X2, [X3, #5]               // Dirección no alineada

// Direcciones fuera de rango
STUR X4, [XZR, #0xFFFFFFFF]     // Dirección fuera de rango
LDUR X5, [XZR, #0xFFFFFFFF]     // Dirección fuera de rango

// Direcciones alineadas en los límites del rango permitido
STUR X6, [XZR, #0]              // Dirección base (inicio de memoria)
LDUR X7, [XZR, #0xFFFFFFF8]     // Dirección cerca del límite superior

// Acceso a direcciones negativas (si el simulador lo permite)
STUR X8, [XZR, #-1]             // Dirección negativa (caso borde)

// Acceso a direcciones con desplazamientos grandes
STUR X9, [X10, #4096]           // Desplazamiento grande
LDUR X11, [X12, #-4096]         // Desplazamiento negativo grande

HLT 0                           // Detener la ejecución
