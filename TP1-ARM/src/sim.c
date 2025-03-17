// sim.c
#include "shell.h"
#include <stdio.h>
#include <stdint.h>

// Función auxiliar para actualizar las banderas N y Z.
// Se asume que C y V siempre son 0.
static void update_flags(int64_t result) {
    NEXT_STATE.FLAG_Z = (result == 0);
    NEXT_STATE.FLAG_N = (result < 0);
}

// La función process_instruction() es llamada por el shell para simular
// la ejecución de una única instrucción.
void process_instruction() {
    // Lee la instrucción de 32 bits de memoria en la dirección actual del PC.
    uint32_t inst = mem_read_32(CURRENT_STATE.PC);

    // Por defecto, se avanza el PC 4 bytes (tamaño de instrucción)
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;   //preguntr si siempre

    // Se extrae un campo de 12 bits que, en este ejemplo, usaremos como opcode.
    uint32_t opcode = (inst >> 20) & 0xFFF;  // desde el 24 o desde el 20?
    printf("Opcode: 0x%03x\n", opcode);

    // Para facilitar la lectura se muestran (opcionalmente) algunos valores:
    // printf("PC: 0x%lx, Instrucción: 0x%08x, Opcode: 0x%03x\n",
    //        CURRENT_STATE.PC, inst, opcode);

    switch (opcode) {
        // --- Instrucciones de detención ---
        //0xd4400000
        

        case 0xd44: { // HLT: hlt 0. Detiene la simulación.
            RUN_BIT = 0;
            break;
        }

        
        
        // --- Instrucciones aritméticas (con actualización de flags) ---
        case 0xb10: { 
            // ADDS Immediate: adds Xd, Xn, imm
            // Se asume:
            // - Bits [4:0]: Rd
            // - Bits [9:5]: Rn
            // - Bits [21:10]: Inmediato (imm12)
            uint32_t imm12 = (inst >> 10) & 0xFFF;
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t result = op1 + imm12;
            NEXT_STATE.REGS[Rd] = result;
            update_flags(result);
            break;
        }
        case 0x458: { 
            // ADDS Extended Register: adds Xd, Xn, Xm
            // Se asume:
            // - Bits [4:0]: Rd
            // - Bits [9:5]: Rn
            // - Bits [20:16]: Rm
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            int64_t result = op1 + op2;
            NEXT_STATE.REGS[Rd] = result;
            update_flags(result);
            break;
        }
        case 0x650: { 
            // SUBS Immediate: subs Xd, Xn, imm
            uint32_t imm12 = (inst >> 10) & 0xFFF;
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t result = op1 - imm12;
            NEXT_STATE.REGS[Rd] = result;
            update_flags(result);
            break;
        }
        case 0x658: { 
            // SUBS Extended Register: subs Xd, Xn, Xm
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            int64_t result = op1 - op2;
            NEXT_STATE.REGS[Rd] = result;
            update_flags(result);
            break;
        }
        case 0x3B0: { 
            // CMP Extended Register: cmp Xn, Xm (solo actualiza flags)
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            int64_t result = op1 - op2;
            update_flags(result);
            break;
        }
        
        // --- Instrucciones lógicas ---
        case 0x1A0: { 
            // ANDS Shifted Register: ands Xd, Xn, Xm
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            int64_t result = op1 & op2;
            NEXT_STATE.REGS[Rd] = result;
            update_flags(result);
            break;
        }
        case 0x1A1: { 
            // EOR Shifted Register: eor Xd, Xn, Xm
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            NEXT_STATE.REGS[Rd] = op1 ^ op2;
            break;
        }
        case 0x1A2: { 
            // ORR Shifted Register: orr Xd, Xn, Xm
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            NEXT_STATE.REGS[Rd] = op1 | op2;
            break;
        }
        
        // --- Instrucciones de salto ---
        case 0x0A0: { 
            // B Unconditional: branch con offset inmediato (campo imm26)
            int32_t imm26 = inst & 0x03FFFFFF;
            if (imm26 & (1 << 25))
                imm26 |= 0xFC000000; // extensión de signo
            int32_t offset = imm26 << 2;
            NEXT_STATE.PC = CURRENT_STATE.PC + offset;
            break;
        }
        case 0x0A1: { 
            // BR: branch a la dirección en el registro Xn
            uint32_t Rn = inst & 0x1F;
            NEXT_STATE.PC = CURRENT_STATE.REGS[Rn];
            break;
        }
        case 0x0A2: { 
            // BEQ: branch condicional si Z == 1
            int32_t imm26 = inst & 0x03FFFFFF;
            if (imm26 & (1 << 25))
                imm26 |= 0xFC000000;
            int32_t offset = imm26 << 2;
            if (CURRENT_STATE.FLAG_Z)
                NEXT_STATE.PC = CURRENT_STATE.PC + offset;
            break;
        }
        case 0x0A3: { 
            // BNE: branch condicional si Z == 0
            int32_t imm26 = inst & 0x03FFFFFF;
            if (imm26 & (1 << 25))
                imm26 |= 0xFC000000;
            int32_t offset = imm26 << 2;
            if (!CURRENT_STATE.FLAG_Z)
                NEXT_STATE.PC = CURRENT_STATE.PC + offset;
            break;
        }
        
        // --- Instrucciones de desplazamiento ---
        case 0x5B0: { 
            // LSL Immediate: Logical Shift Left: lsl Xd, Xn, shamt
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t shamt = (inst >> 10) & 0x3F; // 6 bits
            int64_t op = CURRENT_STATE.REGS[Rn];
            NEXT_STATE.REGS[Rd] = op << shamt;
            break;
        }
        case 0x5B1: { 
            // LSR Immediate: Logical Shift Right: lsr Xd, Xn, shamt
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t shamt = (inst >> 10) & 0x3F;
            int64_t op = CURRENT_STATE.REGS[Rn];
            NEXT_STATE.REGS[Rd] = ((uint64_t)op) >> shamt;
            break;
        }
        
        // --- Instrucciones de carga/almacenamiento ---
        case 0x7B0: { 
            // STUR: store register to memory: stur Xt, [Xn, #imm]
            // Se asume:
            // - Bits [4:0]: Rt
            // - Bits [9:5]: Rn (base)
            // - Bits [21:10]: Inmediato (offset)
            uint32_t Rt = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t imm = (inst >> 10) & 0xFFF;
            uint64_t addr = CURRENT_STATE.REGS[Rn] + imm;
            uint32_t value = (uint32_t)(CURRENT_STATE.REGS[Rt] & 0xFFFFFFFF);
            mem_write_32(addr, value);
            break;
        }
        case 0x7B3: { 
            // LDUR: load register from memory: ldur Xt, [Xn, #imm]
            uint32_t Rt = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t imm = (inst >> 10) & 0xFFF;
            uint64_t addr = CURRENT_STATE.REGS[Rn] + imm;
            uint32_t value = mem_read_32(addr);
            NEXT_STATE.REGS[Rt] = value; // se extiende a 64 bits (parte alta cero)
            break;
        }
        
        // --- Instrucciones de movimiento inmediato ---
        case 0x2B0: { 
            // MOVZ: movz Xd, imm16 (solo se implementa con shift 0)
            uint32_t Rd = inst & 0x1F;
            uint32_t imm16 = (inst >> 5) & 0xFFFF;
            NEXT_STATE.REGS[Rd] = imm16;
            break;
        }
        
        // --- Instrucciones de multiplicación ---
        case 0x2F0: { 
            // MUL: mul Xd, Xn, Xm
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            NEXT_STATE.REGS[Rd] = op1 * op2;
            break;
        }
        
        // --- Instrucciones de branch condicional con comparación implícita ---
        case 0x4B0: { 
            // CBZ: compare and branch if zero: cbz Xn, label
            uint32_t Rt = inst & 0x1F;
            int32_t imm19 = (inst >> 5) & 0x7FFFF;
            if (imm19 & (1 << 18))
                imm19 |= 0xFFF00000;
            int32_t offset = imm19 << 2;
            if (CURRENT_STATE.REGS[Rt] == 0)
                NEXT_STATE.PC = CURRENT_STATE.PC + offset;
            break;
        }
        case 0x4B1: { 
            // CBNZ: compare and branch if non-zero: cbnz Xn, label
            uint32_t Rt = inst & 0x1F;
            int32_t imm19 = (inst >> 5) & 0x7FFFF;
            if (imm19 & (1 << 18))
                imm19 |= 0xFFF00000;
            int32_t offset = imm19 << 2;
            if (CURRENT_STATE.REGS[Rt] != 0)
                NEXT_STATE.PC = CURRENT_STATE.PC + offset;
            break;
        }
        
        default:
            // Si la instrucción no está implementada, se muestra un mensaje y se detiene la simulación.
            printf("Instrucción no implementada: 0x%08x\n", inst);
            RUN_BIT = 0;
            break;
    }
}
