.text
MOV X0, #0                      // Configurar registro en cero
CBZ X0, label_cbz_zero          // Saltar si X0 es cero
NOP                             // No debería ejecutarse si CBZ funciona correctamente

label_cbz_zero:
MOV X2, #42                     // Indicar que se tomó el salto CBZ

MOV X1, #1                      // Configurar registro en no cero
CBNZ X1, label_cbnz_nonzero     // Saltar si X1 no es cero
NOP                             // No debería ejecutarse si CBNZ funciona correctamente

label_cbnz_nonzero:
MOV X3, #99                     // Indicar que se tomó el salto CBNZ

HLT 0                           // Detener la ejecución
