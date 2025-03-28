.text
// Caso 1: Saltar si Z está establecida (igualdad)
CMP X0, X0                      // Configurar bandera Z (igualdad)
BEQ label_equal                 // Saltar si Z está establecida

label_equal:
CMP X1, #1                      // Configurar bandera N (negativo)
BLT label_less                  // Saltar si N está establecida (X1 < 1)

label_less:
CMP X2, #0                      // Configurar bandera Z (igualdad) o N (negativo)
BNE label_not_equal             // Saltar si Z no está establecida (X2 != 0)

label_not_equal:
CMP X3, #0
BGE label_greater_equal         // Saltar si N no está establecida (X3 >= 0)

label_greater_equal:
CMP X4, #0
BLE label_less_equal            // Saltar si N está establecida o Z está establecida (X4 <= 0)

label_less_equal:
B label_unconditional           // Salto incondicional

label_unconditional:
HLT 0                           // Detener la ejecución
