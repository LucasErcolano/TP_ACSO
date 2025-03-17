// sim.c
#include "shell.h"
#include <stdio.h>
#include <stdint.h>

// Helper function to update the NZ flags (assumes C and V remain 0)
static void update_flags(int64_t result) {
    NEXT_STATE.FLAG_Z = (result == 0);
    NEXT_STATE.FLAG_N = (result < 0);
}

// process_instruction() is called by the shell to simulate execution of a single instruction.
void process_instruction() {
    // Read the 32-bit instruction from memory at the current PC.
    uint32_t inst = mem_read_32(CURRENT_STATE.PC);

    // By default, advance PC by 4 bytes.
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;

    // Extract the top 8 bits to use as the opcode.
    uint32_t opcode = (inst >> 24) & 0xFF;

    // For clarity, note that in this updated version the opcode values have been chosen
    // to roughly match the fixed AArch64 encoding fields when always operating with sf=1.
    // The mapping used here is as follows (original -> new):
    //   HLT                0x6A2 -> 0xD4
    //   ADDS Immediate     0x450 -> 0xB1
    //   ADDS Reg (extended)0x458 -> 0xB9
    //   SUBS Immediate     0x650 -> 0xF1
    //   SUBS Reg (extended)0x658 -> 0xF9
    //   CMP Reg            0x3B0 -> 0x79   (alias for SUBS with Rd==XZR)
    //   ANDS Shifted Reg   0x1A0 -> 0xA0
    //   EOR Shifted Reg    0x1A1 -> 0xA1
    //   ORR Shifted Reg    0x1A2 -> 0xA2
    //   B (unconditional)  0x0A0 -> 0x14
    //   BR                 0x0A1 -> 0xD6
    //   BEQ                0x0A2 -> 0x54
    //   BNE                0x0A3 -> 0x55
    //   LSL Immediate      0x5B0 -> 0xD0
    //   LSR Immediate      0x5B1 -> 0xD1
    //   STUR (store)       0x7B0 -> 0xF8
    //   LDUR (load)        0x7B3 -> 0xFA
    //   MOVZ Immediate     0x2B0 -> 0xD2
    //   MUL                0x2F0 -> 0x9B
    //   CBZ                0x4B0 -> 0xB4
    //   CBNZ               0x4B1 -> 0xB5

    switch (opcode) {
        // --- Exception / system instructions ---
        case 0xD4: { // HLT: halt simulation.
            RUN_BIT = 0;
            break;
        }
        
        // --- Arithmetic instructions with flag update ---
        case 0xB1: { 
            // ADDS Immediate: adds Xd, Xn, #imm{, LSL #12}
            // Format (AArch64 ADD immediate with flags):
            //   bits[31] sf=1, bit[30] op=0, bit[29] S=1,
            //   bit[22] is a shift flag, bits[21:10] immediate (imm12),
            //   bits[9:5] Rn, bits[4:0] Rd.
            uint32_t imm12 = (inst >> 10) & 0xFFF;
            uint32_t shift = (inst >> 22) & 0x1;
            if (shift)
                imm12 = imm12 << 12; // if shift==1, immediate is scaled by 4096
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t result = op1 + imm12;
            NEXT_STATE.REGS[Rd] = result;
            update_flags(result);
            break;
        }
        case 0xB9: { 
            // ADDS Extended Register (shifted register): adds Xd, Xn, Xm{, LSL #shift}
            // Format (AArch64 ADD (shifted register) with flags):
            //   bits[31] sf=1, bit[30] op=0, bit[29] S=1,
            //   bits[4:0] Rd, bits[9:5] Rn, bits[20:16] Rm.
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            // (For simplicity, we assume the shift amount is 0.)
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            int64_t result = op1 + op2;
            NEXT_STATE.REGS[Rd] = result;
            update_flags(result);
            break;
        }
        case 0xF1: { 
            // SUBS Immediate: subs Xd, Xn, #imm{, LSL #12}
            uint32_t imm12 = (inst >> 10) & 0xFFF;
            uint32_t shift = (inst >> 22) & 0x1;
            if (shift)
                imm12 = imm12 << 12;
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t result = op1 - imm12;
            NEXT_STATE.REGS[Rd] = result;
            update_flags(result);
            break;
        }
        case 0xF9: { 
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
            RUN_BIT = 0;
            break;
    }
}
