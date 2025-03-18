#include "shell.h"
#include <stdio.h>
#include <stdint.h>

static void update_flags(int64_t result) {
    NEXT_STATE.FLAG_Z = (result == 0);   
    NEXT_STATE.FLAG_N = (result < 0);
    //CURRENT_STATE.FLAG_N = (result >> 63) & 1; ?
    //current o next flag?
}

// Helper function: Extend a 64-bit value based on the option and apply left shift by imm3.
static inline uint64_t extend_register(uint64_t value, int option, int imm3) {
    uint64_t result;
    switch (option) {
        case 0: // UXTB: extract 8 bits, zero-extend
            result = value & 0xFF;
            break;
        case 1: // UXTH: extract 16 bits, zero-extend
            result = value & 0xFFFF;
            break;
        case 2: // UXTW: extract 32 bits, zero-extend
            result = value & 0xFFFFFFFF;
            break;
        case 3: // UXTX: no extension needed
            result = value;
            break;
        case 4: { // SXTB: extract 8 bits, sign-extend
            int8_t tmp = value & 0xFF;
            result = (uint64_t)tmp;
            break;
        }
        case 5: { // SXTH: extract 16 bits, sign-extend
            int16_t tmp = value & 0xFFFF;
            result = (uint64_t)tmp;
            break;
        }
        case 6: { // SXTW: extract 32 bits, sign-extend
            int32_t tmp = value & 0xFFFFFFFF;
            result = (uint64_t)tmp;
            break;
        }
        case 7: // SXTX: no extension needed (already 64 bits)
            result = value;
            break;
        default:
            result = value;
            break;
    }
    return result << imm3;
}

void execute_ADDS_immediate(int d, int n, uint32_t imm12, int shift) {
    uint64_t operand1 = (n == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[n];
    uint64_t imm = (shift == 1) ? (imm12 << 12) : imm12;
    uint64_t result = operand1 + imm;
    
    update_flags(result);
    CURRENT_STATE.REGS[d] = result;
}

void execute_ADDS_extended(int d, int n, int m, int option, int imm3) {
    uint64_t operand1 = (n == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[n];
    uint64_t operand2 = extend_register(CURRENT_STATE.REGS[m], option, imm3);
    uint64_t result = operand1 + operand2;
    
    update_flags(result);
    CURRENT_STATE.REGS[d] = result;
}

void execute_SUBS_immediate(int d, int n, uint32_t imm12, int shift) {
    uint64_t operand1 = (n == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[n];
    uint64_t imm;
    
    // Decode the immediate value based on the shift flag.
    if (shift == 0) {
        imm = imm12;
    } else if (shift == 1) {
        imm = imm12 << 12;
    } else {
        // Reserved shift value; handle as needed (here, default to 0).
        imm = 0;
    }
    
    // Compute subtraction via two's complement:
    //   result = operand1 - imm = operand1 + (~imm) + 1
    uint64_t result = operand1 + (~imm) + 1;
    
    update_flags(result);
    CURRENT_STATE.REGS[d] = result;
}

void execute_SUBS_extended(int d, int n, int m, int option, int imm3) {
    // Read operand1; use SP (assumed stored in REGS[31]) if n==31.
    uint64_t operand1 = (n == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[n];
    // Compute the extended value from register m.
    uint64_t operand2 = extend_register(CURRENT_STATE.REGS[m], option, imm3);
    // For subtraction, compute operand1 - operand2 as:
    //   operand1 + (~operand2) + 1
    uint64_t result = operand1 + (~operand2) + 1;
    
    // Update flags: N is the most-significant bit and Z is set if result==0.
    update_flags(result);

    // Write the result to the destination register.
    CURRENT_STATE.REGS[d] = result;
}

void process_instruction() {
    uint32_t inst = mem_read_32(CURRENT_STATE.PC);

    NEXT_STATE.PC = CURRENT_STATE.PC + 4;

    // Extract the top 8 bits to use as the opcode.
    uint32_t opcode = (inst >> 24) & 0xFF;
    // Print the opcode for debugging purposes.
    printf("Opcode: 0x%02x\n", opcode);

    switch (opcode) {
        case 0xD4: { // HLT: halt simulation.
            RUN_BIT = 0;
            break;
        }
        
        case 0xB1: {  // ADDS immediate 
            uint64_t imm12 = (inst >> 10) & 0xFFF;
            uint32_t shift = (inst >> 22) & 0x1;
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            execute_ADDS_immediate(Rd, Rn, imm12, shift);
            break;
        }

        case 0xAB: { // ADDS Extended Register
            uint32_t Rd     = inst & 0x1F;
            uint32_t Rn     = (inst >> 5) & 0x1F;
            uint32_t imm3   = (inst >> 10) & 0x7;
            uint32_t option = (inst >> 13) & 0x7;
            uint32_t Rm     = (inst >> 16) & 0x1F;
            // Call the helper for ADDS (extended register)
            execute_ADDS_extended(Rd, Rn, Rm, option, imm3);
            break;
        }

        case 0xF1: { // SUBS immediate
            uint64_t imm12 = (inst >> 10) & 0xFFF;
            uint32_t shift = (inst >> 22) & 0x1;
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            execute_SUBS_immediate(Rd, Rn, imm12, shift);
            break;
        }

        case 0xEB: { // SUBS Extended Register
            uint32_t Rd     = inst & 0x1F;
            uint32_t Rn     = (inst >> 5) & 0x1F;
            uint32_t imm3   = (inst >> 10) & 0x7;
            uint32_t option = (inst >> 13) & 0x7;
            uint32_t Rm     = (inst >> 16) & 0x1F;
            execute_SUBS_extended(Rd, Rn, Rm, option, imm3);
            break;
        }

        case 0x79: { 
            // CMP Extended Register: cmp Xn, Xm (alias for SUBS with Rd==XZR)
            // Only update flags.
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            int64_t result = op1 - op2;
            update_flags(result);
            break;
        }
        
        // --- Logical instructions ---
        case 0xA0: { 
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
        case 0xA1: { 
            // EOR Shifted Register: eor Xd, Xn, Xm
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            NEXT_STATE.REGS[Rd] = op1 ^ op2;
            break;
        }
        case 0xA2: { 
            // ORR Shifted Register: orr Xd, Xn, Xm
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            NEXT_STATE.REGS[Rd] = op1 | op2;
            break;
        }
        
        // --- Branch instructions ---
        case 0x14: { 
            // B Unconditional: branch with immediate offset (imm26)
            int32_t imm26 = inst & 0x03FFFFFF;
            if (imm26 & (1 << 25))
                imm26 |= 0xFC000000; // sign-extension from bit 25
            int32_t offset = imm26 << 2;
            NEXT_STATE.PC = CURRENT_STATE.PC + offset;
            break;
        }
        case 0xD6: { 
            // BR: branch to address in register Xn
            uint32_t Rn = inst & 0x1F;
            NEXT_STATE.PC = CURRENT_STATE.REGS[Rn];
            break;
        }
        case 0x54: { 
            // BEQ: branch conditionally if Z==1
            int32_t imm19 = inst & 0x7FFFF; // 19-bit immediate
            if (imm19 & (1 << 18))
                imm19 |= 0xFFF00000;
            int32_t offset = imm19 << 2;
            if (CURRENT_STATE.FLAG_Z)
                NEXT_STATE.PC = CURRENT_STATE.PC + offset;
            break;
        }
        case 0x55: { 
            // BNE: branch conditionally if Z==0
            int32_t imm19 = inst & 0x7FFFF;
            if (imm19 & (1 << 18))
                imm19 |= 0xFFF00000;
            int32_t offset = imm19 << 2;
            if (!CURRENT_STATE.FLAG_Z)
                NEXT_STATE.PC = CURRENT_STATE.PC + offset;
            break;
        }
        
        // --- Shift instructions (immediate) ---
        case 0xD0: { 
            // LSL Immediate: logical shift left: lsl Xd, Xn, #shamt
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            // For simplicity, we assume the shamt is in bits [10:5] (6 bits)
            uint32_t shamt = (inst >> 10) & 0x3F;
            int64_t op = CURRENT_STATE.REGS[Rn];
            NEXT_STATE.REGS[Rd] = op << shamt;
            break;
        }
        case 0xD1: { 
            // LSR Immediate: logical shift right: lsr Xd, Xn, #shamt
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t shamt = (inst >> 10) & 0x3F;
            int64_t op = CURRENT_STATE.REGS[Rn];
            NEXT_STATE.REGS[Rd] = ((uint64_t)op) >> shamt;
            break;
        }
        
        // --- Load/Store instructions ---
        case 0xF8: { 
            // STUR: store register to memory: stur Xt, [Xn, #imm]
            // Format (AArch64 STUR for 64-bit register):
            //   bits[31:22] fixed, bits[21:12] immediate (signed 9-bit),
            //   bits[9:5] Rn (base), bits[4:0] Rt.
            // Here we assume the immediate is in bits [21:12] (9 bits) and is in bytes.
            int32_t imm9 = (inst >> 12) & 0x1FF;
            // Sign-extend from bit 8 of the 9-bit field:
            if (imm9 & (1 << 8))
                imm9 |= 0xFFFFFE00;
            uint32_t Rt = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
            uint64_t value = CURRENT_STATE.REGS[Rt];  // 64-bit value (sf=1)
            mem_write_32(addr, value);
            break;
        }
        case 0xFA: { 
            // LDUR: load register from memory: ldur Xt, [Xn, #imm]
            int32_t imm9 = (inst >> 12) & 0x1FF;
            if (imm9 & (1 << 8))
                imm9 |= 0xFFFFFE00;
            uint32_t Rt = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
            uint64_t value = mem_read_32(addr);
            NEXT_STATE.REGS[Rt] = value;
            break;
        }
        
        // --- Move immediate ---
        case 0xD2: { 
            // MOVZ: movz Xd, imm16 (only shift 0 supported here)
            // Format (AArch64 MOVZ): bits[31:29]: sf=1, fixed bits, bits[28:23] include shift amount,
            // bits[22:5]: imm16, bits[4:0]: Rd.
            uint32_t Rd = inst & 0x1F;
            uint32_t imm16 = (inst >> 5) & 0xFFFF;
            NEXT_STATE.REGS[Rd] = imm16;
            break;
        }
        
        // --- Multiplication ---
        case 0x9B: { 
            // MUL: mul Xd, Xn, Xm
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            NEXT_STATE.REGS[Rd] = op1 * op2;
            break;
        }
        
        // --- Compare and branch (immediate) ---
        case 0xB4: { 
            // CBZ: compare and branch if zero: cbz Xn, label
            // Format (AArch64 CBZ): bits[31:24] fixed, bits[23:5] immediate (19 bits),
            // bits[4:0] Rt.
            uint32_t Rt = inst & 0x1F;
            int32_t imm19 = (inst >> 5) & 0x7FFFF;
            if (imm19 & (1 << 18))
                imm19 |= 0xFFF00000;
            int32_t offset = imm19 << 2;
            if (CURRENT_STATE.REGS[Rt] == 0)
                NEXT_STATE.PC = CURRENT_STATE.PC + offset;
            break;
        }
        case 0xB5: { 
            // CBNZ: compare and branch if nonzero: cbnz Xn, label
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
            // If the instruction is not implemented, print a message and halt simulation.
            printf("Instruction not implemented: 0x%08x\n", inst);
            break;
    }
}
