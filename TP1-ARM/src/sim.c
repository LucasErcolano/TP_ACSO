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

int64_t sign_extend(int64_t value, int bits) {
    int64_t mask = (int64_t)1 << (bits - 1);
    return (value ^ mask) - mask; 
}

void process_instruction() {
    uint32_t inst = mem_read_32(CURRENT_STATE.PC);

    NEXT_STATE.PC = CURRENT_STATE.PC + 4;

    uint32_t opcode8 = (inst >> 24) & 0xFF;

    switch (opcode8) { 
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

        case 0xF1: { // SUBS/CMP immediate
            uint64_t imm12 = (inst >> 10) & 0xFFF;
            uint32_t shift = (inst >> 22) & 0x3;
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint64_t operand1 = (Rn == 31) ? SP : CURRENT_STATE.REGS[Rn];
            uint64_t operand2;
            
            if (shift == 0) {
                operand2 = imm12;
            } else if (shift == 1) {
                operand2 = imm12 << 12;
            } else {
                return;
            }
            uint64_t result = operand1 - operand2;
            update_flags(result);
            if (Rd != 31) {
                CURRENT_STATE.REGS[Rd] = result;
            }
            break;
        }
        
        case 0xEB: { // SUBS/CMP Extended Register
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t imm3 = (inst >> 10) & 0x7;
            uint32_t option = (inst >> 13) & 0x7;
            uint32_t Rm = (inst >> 16) & 0x1F;
            
            uint64_t operand1 = (Rn == 31) ? SP : CURRENT_STATE.REGS[Rn];
            uint64_t operand2 = extend_register(CURRENT_STATE.REGS[Rm], option, imm3);
            uint64_t result = operand1 - operand2;
            update_flags(result);            
            if (Rd != 31) {
                CURRENT_STATE.REGS[Rd] = result;
            }
            break;
        }

        case 0xEA: { // ANDS (shifted register)
            uint8_t shift  = (inst >> 22) & 0x3;
            uint8_t rm     = (inst >> 16) & 0x1F;
            uint8_t imm6   = (inst >> 10) & 0x3F;
            uint8_t rn     = (inst >> 5)  & 0x1F;
            uint8_t rd     = inst & 0x1F;
        
            uint64_t operand1 = CURRENT_STATE.REGS[rn];
            uint64_t operand2 = CURRENT_STATE.REGS[rm];
            switch (shift) {
                case 0:
                    operand2 = operand2 << imm6;
                    break;
                case 1:
                    operand2 = operand2 >> imm6;
                    break;
                case 2:
                    operand2 = ((int64_t)operand2) >> imm6;
                    break;
                case 3:
                    operand2 = (operand2 >> imm6) | (operand2 << (64 - imm6));
                    break;
            }
            uint64_t result = operand1 & operand2;
            update_flags(result);
            NEXT_STATE.REGS[rd] = result;
            break;
        }
        
        case 0xCA: { // EOR shifted register
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t imm6 = (inst >> 10) & 0x3F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            uint32_t shift = (inst >> 22) & 0x3;
            
            uint64_t operand1 = CURRENT_STATE.REGS[Rn];
            uint64_t operand2 = CURRENT_STATE.REGS[Rm];
            switch (shift) {
                case 0:
                    operand2 = (imm6 == 0) ? operand2 : (operand2 << imm6);
                    break;
                case 1:
                    operand2 = (imm6 == 0) ? operand2 : (operand2 >> imm6);
                    break;
                case 2:
                    operand2 = (imm6 == 0) ? operand2 : ((int64_t)operand2 >> imm6);
                    break;
                case 3:
                    if (imm6 == 0) {
                        operand2 = operand2;
                    } else {
                        operand2 = (operand2 >> imm6) | (operand2 << (64 - imm6));
                    }
                    break;
            }
            uint64_t result = operand1 ^ operand2;
            CURRENT_STATE.REGS[Rd] = result;
            update_flags(result);
            break;
        }

        case 0xAA: { // ORR Shifted Register
            uint32_t Rd = inst & 0x1F;
            uint32_t Rn = (inst >> 5) & 0x1F;
            uint32_t Rm = (inst >> 16) & 0x1F;
            int64_t op1 = CURRENT_STATE.REGS[Rn];
            int64_t op2 = CURRENT_STATE.REGS[Rm];
            NEXT_STATE.REGS[Rd] = op1 | op2;
            break;
        }
        default:
        printf("Instruction not implemented: 0x%08x\n", inst);
        break;
    }
    
    uint32_t opcode6 = (inst >> 26) & 0x3F;    
    switch (opcode6) {
        case 0x14: { // B
            int32_t imm26 = inst & 0x3FFFFFF;
            int64_t offset = ((int64_t)(imm26 << 6)) >> 4;
            uint64_t target = CURRENT_STATE.PC + offset;
            CURRENT_STATE.PC = target;
            //branch_taken = 1;
            break;
        }
        default:
        printf("Instruction not implemented: 0x%08x\n", inst);
        break;
    }

    uint32_t opcode16 = (inst >> 10) & 0xFFFF;    
    switch (opcode16) {
        case 0xD61F: { // BR
            uint32_t Rn = (inst >> 5) & 0x1F;
            
            uint64_t target = CURRENT_STATE.REGS[Rn];
            CURRENT_STATE.PC = target;
            //branch_taken = 1;
            break;
        }
        default:
        printf("Instruction not implemented: 0x%08x\n", inst);
        break;
    }
    uint32_t opcode11 = (inst >> 21) & 0x7FF;
    switch (opcode11) {
        case 0x7C0: {  // STUR
            int32_t imm9 = (inst >> 12) & 0x1FF;
            int32_t Rn = (inst >> 5) & 0x1F;
            int32_t Rt = inst & 0x1F;

            imm9 = sign_extend(imm9, 64);
        
            uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
            uint64_t value = CURRENT_STATE.REGS[Rt];
            mem_write_64(addr, value);
            break;
        }

        case 0x1C0: {  // STURB
            int64_t imm9 = (inst >> 12) & 0x1FF; 
            int64_t Rn = (inst >> 5) & 0x1F;     
            int64_t Rt = inst & 0x1F;            
        
            imm9 = sign_extend(imm9, 64);
        
            uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;  
            uint8_t value = (uint8_t) CURRENT_STATE.REGS[Rt];
        
            mem_write_8(addr, value);
        
            break;
        }
        
        case 0x3C0: {  // STURH
            int64_t imm9 = (inst >> 12) & 0x1FF; 
            int64_t Rn = (inst >> 5) & 0x1F;     
            int64_t Rt = inst & 0x1F;            
        
            imm9 = sign_extend(imm9, 64);
        
            uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;  
            uint16_t value = (uint16_t) CURRENT_STATE.REGS[Rt];
    
            mem_write_16(addr, value);
        
            break;
        }

        case 0x7C2: {  // LDUR
            int32_t imm9 = (inst >> 12) & 0x1FF;
            int32_t Rn = (inst >> 5) & 0x1F;    
            int32_t Rt = inst & 0x1F;           
        
            imm9 = sign_extend(imm9, 64); 
        
            uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9; 
            uint64_t value = mem_read_64(addr);          
        
            CURRENT_STATE.REGS[Rt] = value;
            break;
        }

        case 0x1C2: {  // LDURB
            int32_t imm9 = (inst >> 12) & 0x1FF;
            int32_t Rn = (inst >> 5) & 0x1F;    
            int32_t Rt = inst & 0x1F;           
        
            imm9 = sign_extend(imm9, 64); 
        
            uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9; 
            uint8_t value = mem_read_8(addr);             
        
            CURRENT_STATE.REGS[Rt] = (uint64_t)value; 
            break;
        }
        
        case 0x5C2: {  // LDURH
            int32_t imm9 = (inst >> 12) & 0x1FF;
            int32_t Rn = (inst >> 5) & 0x1F;
            int32_t Rt = inst & 0x1F;
        
            imm9 = sign_extend(imm9, 64);
        
            uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
            uint16_t value = mem_read_16(addr);
        
            CURRENT_STATE.REGS[Rt] = (uint64_t)value;
            break;
        }
        default:
            printf("Instruction not implemented: 0x%08x\n", inst);
            break;
        
    }

}
