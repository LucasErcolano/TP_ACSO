    .text
    .global _start
_start:
    // --------------------
    // Test BEQ (Branch if Equal)
    // Configuración: X1 = 5, X2 = 5  => 5 - 5 = 0, Z = 1
    adds    X1, XZR, 5       // X1 = 5
    adds    X2, XZR, 5       // X2 = 5
    subs    X31, X1, X2      // CMP simulado: X31 (XZR) = X1 - X2; flags: Z = 1
    beq     beq_taken        // Debería saltar si Z==1
    // Si no salta, se marca fallo:
beq_fail:
    adds    X0, XZR, 1       // X0 = 1 (fallo BEQ)
    b       beq_end
beq_taken:
    adds    X0, XZR, 10      // X0 = 10 (éxito BEQ)
beq_end:

    // --------------------
    // Test BNE (Branch if Not Equal)
    // Configuración: X1 = 5, X2 = 6  => 5 - 6 = -1, Z = 0
    adds    X1, XZR, 5       // X1 = 5
    adds    X2, XZR, 6       // X2 = 6
    subs    X31, X1, X2      // Resultado negativo, Z = 0
    bne     bne_taken        // Debería saltar si Z==0
    // Si no salta, se marca fallo:
bne_fail:
    adds    X1, XZR, 1       // X1 = 1 (fallo BNE)
    b       bne_end
bne_taken:
    adds    X1, XZR, 20      // X1 = 20 (éxito BNE)
bne_end:

    // --------------------
    // Test BGE (Branch if Greater or Equal)
    // Condición: se salta si N==0 (asumiendo V==0).
    // Configuración: X1 = 6, X2 = 5  => 6 - 5 = 1, N = 0
    adds    X1, XZR, 6       // X1 = 6
    adds    X2, XZR, 5       // X2 = 5
    subs    X31, X1, X2      // Resultado positivo: N = 0
    bge     bge_taken        // Debería saltar si N==0
    // Si no salta, se marca fallo:
bge_fail:
    adds    X2, XZR, 1       // X2 = 1 (fallo BGE)
    b       bge_end
bge_taken:
    adds    X2, XZR, 30      // X2 = 30 (éxito BGE)
bge_end:

    // --------------------
    // Test BLT (Branch if Less Than)
    // Condición: se salta si N != 0.
    // Configuración: X1 = 5, X2 = 6  => 5 - 6 = -1, N = 1
    adds    X1, XZR, 5       // X1 = 5
    adds    X2, XZR, 6       // X2 = 6
    subs    X31, X1, X2      // Resultado negativo: N = 1
    blt     blt_taken        // Debería saltar si N != 0
    // Si no salta, se marca fallo:
blt_fail:
    adds    X3, XZR, 1       // X3 = 1 (fallo BLT)
    b       blt_end
blt_taken:
    adds    X3, XZR, 40      // X3 = 40 (éxito BLT)
blt_end:

    // --------------------
    // Test BGT (Branch if Greater Than)
    // Condición: se salta si Z == 0 y N == 0.
    // Configuración: X1 = 6, X2 = 5  => 6 - 5 = 1, (Z = 0, N = 0)
    adds    X1, XZR, 6       // X1 = 6
    adds    X2, XZR, 5       // X2 = 5
    subs    X31, X1, X2      // Resultado positivo: Z = 0, N = 0
    bgt     bgt_taken        // Debería saltar
    // Si no salta, se marca fallo:
bgt_fail:
    adds    X4, XZR, 1       // X4 = 1 (fallo BGT)
    b       bgt_end
bgt_taken:
    adds    X4, XZR, 50      // X4 = 50 (éxito BGT)
bgt_end:

    // --------------------
    // Test BLE (Branch if Less or Equal)
    // Condición: se salta si Z == 1 o N != 0.
    // Se realizan dos sub-tests:
    // Sub-test 1: Igualdad
    adds    X1, XZR, 5       // X1 = 5
    adds    X2, XZR, 5       // X2 = 5
    subs    X31, X1, X2      // Resultado = 0, Z = 1
    ble     ble_taken_eq     // Debería saltar (por Z == 1)
    // Si no salta, se marca fallo:
ble_fail_eq:
    adds    X5, XZR, 1       // X5 = 1 (fallo BLE, igualdad)
    b       ble_end_eq
ble_taken_eq:
    adds    X5, XZR, 60      // X5 = 60 (éxito BLE, igualdad)
ble_end_eq:
    // Sub-test 2: Menor
    adds    X1, XZR, 5       // X1 = 5
    adds    X2, XZR, 6       // X2 = 6
    subs    X31, X1, X2      // Resultado negativo, N != 0
    ble     ble_taken_lt     // Debería saltar (por N != 0)
    // Si no salta, se marca fallo:
ble_fail_lt:
    adds    X6, XZR, 1       // X6 = 1 (fallo BLE, menor)
    b       ble_end_lt
ble_taken_lt:
    adds    X6, XZR, 70      // X6 = 70 (éxito BLE, menor)
ble_end_lt:

    // --------------------
    // Fin del test: detener la simulación
    hlt 0
