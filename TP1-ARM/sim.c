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
    // Read operand1; use CURRENT_STATE.REGS[31] (assumed stored in REGS[31]) if n==31.
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

uint8_t mem_read_8(uint64_t address) {
    // Determine the aligned 32-bit address that contains our byte
    uint64_t aligned_addr = address & ~0x3;
    
    // Read the entire 32-bit word that contains our byte
    uint32_t word = mem_read_32(aligned_addr);
    
    // Extract the specific byte based on the original address
    int byte_offset = address & 0x3;
    
    // Shift and mask to get the correct byte (little-endian)
    return (word >> (byte_offset * 8)) & 0xFF;
}

uint16_t mem_read_16(uint64_t address) {
    // Check if the address is halfword-aligned
    if (address & 0x1) {
        printf("Warning: Unaligned halfword read at 0x%" PRIx64 "\n", address);
    }
    
    // Determine the aligned 32-bit address that contains our halfword
    uint64_t aligned_addr = address & ~0x3;
    
    // Read the entire 32-bit word
    uint32_t word = mem_read_32(aligned_addr);
    
    // Extract the specific halfword based on the original address
    int halfword_offset = (address & 0x3) / 2;
    
    // Shift and mask to get the correct halfword (little-endian)
    return (word >> (halfword_offset * 16)) & 0xFFFF;
}

uint64_t mem_read_64(uint64_t address) {
    // Check if the address is 8-byte aligned
    if (address & 0x7) {
        printf("Warning: Unaligned 64-bit read at 0x%" PRIx64 "\n", address);
    }
    
    // Read the two 32-bit words that make up our 64-bit value
    uint32_t low_word = mem_read_32(address);
    uint32_t high_word = mem_read_32(address + 4);
    
    // Combine them into a 64-bit value (little-endian)
    return ((uint64_t)high_word << 32) | low_word;
}

void mem_write_8(uint64_t address, uint8_t value) {
    // Determine the aligned 32-bit address that contains our byte
    uint64_t aligned_addr = address & ~0x3;
    
    // Read the existing 32-bit word
    uint32_t word = mem_read_32(aligned_addr);
    
    // Determine which byte to modify
    int byte_offset = address & 0x3;
    
    // Clear the target byte and insert the new value (little-endian)
    uint32_t byte_mask = 0xFF << (byte_offset * 8);
    word = (word & ~byte_mask) | ((value & 0xFF) << (byte_offset * 8));
    
    // Write back the modified word
    mem_write_32(aligned_addr, word);
}

void mem_write_16(uint64_t address, uint16_t value) {
    // Check if the address is halfword-aligned
    if (address & 0x1) {
        printf("Warning: Unaligned halfword write at 0x%" PRIx64 "\n", address);
    }
    
    // Determine the aligned 32-bit address that contains our halfword
    uint64_t aligned_addr = address & ~0x3;
    
    // Read the existing 32-bit word
    uint32_t word = mem_read_32(aligned_addr);
    
    // Determine which halfword to modify
    int halfword_offset = (address & 0x3) / 2;
    
    // Clear the target halfword and insert the new value (little-endian)
    uint32_t halfword_mask = 0xFFFF << (halfword_offset * 16);
    word = (word & ~halfword_mask) | ((value & 0xFFFF) << (halfword_offset * 16));
    
    // Write back the modified word
    mem_write_32(aligned_addr, word);
}

void mem_write_64(uint64_t address, uint64_t value) {
    // Check if the address is 8-byte aligned
    if (address & 0x7) {
        printf("Warning: Unaligned 64-bit write at 0x%" PRIx64 "\n", address);
    }
    
    // Split the 64-bit value into two 32-bit words (little-endian)
    uint32_t low_word = value & 0xFFFFFFFF;
    uint32_t high_word = (value >> 32) & 0xFFFFFFFF;
    
    // Write the two 32-bit words
    mem_write_32(address, low_word);
    mem_write_32(address + 4, high_word);
}

void process_instruction() {
    uint32_t inst = mem_read_32(CURRENT_STATE.PC);
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;

    // Extract different parts of the instruction for pattern matching
    uint32_t opcode8 = (inst >> 24) & 0xFF;
    uint32_t opcode6 = (inst >> 26) & 0x3F;    
    uint32_t opcode16 = (inst >> 10) & 0xFFFF;
    uint32_t opcode11 = (inst >> 21) & 0x7FF;

    // Instruction handled flag to avoid multiple "not implemented" messages
    int instruction_handled = 0;

    // First check opcode8 patterns
    if (opcode8 == 0xD4) { // HLT: halt simulation
        RUN_BIT = 0;
        instruction_handled = 1;
    }
    else if (opcode8 == 0xB1) { // ADDS immediate
        uint64_t imm12 = (inst >> 10) & 0xFFF;
        uint32_t shift = (inst >> 22) & 0x1;
        uint32_t Rd = inst & 0x1F;
        uint32_t Rn = (inst >> 5) & 0x1F;
        execute_ADDS_immediate(Rd, Rn, imm12, shift);
        instruction_handled = 1;
    }
    else if (opcode8 == 0xAB) { // ADDS Extended Register
        uint32_t Rd     = inst & 0x1F;
        uint32_t Rn     = (inst >> 5) & 0x1F;
        uint32_t imm3   = (inst >> 10) & 0x7;
        uint32_t option = (inst >> 13) & 0x7;
        uint32_t Rm     = (inst >> 16) & 0x1F;
        execute_ADDS_extended(Rd, Rn, Rm, option, imm3);
        instruction_handled = 1;
    }
    else if (opcode8 == 0xF1) { // SUBS/CMP immediate
        uint64_t imm12 = (inst >> 10) & 0xFFF;
        uint32_t shift = (inst >> 22) & 0x3;
        uint32_t Rd = inst & 0x1F;
        uint32_t Rn = (inst >> 5) & 0x1F;
        uint64_t operand1 = (Rn == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[Rn];
        uint64_t operand2;
        
        if (shift == 0) {
            operand2 = imm12;
        } else if (shift == 1) {
            operand2 = imm12 << 12;
        } else {
            // Invalid shift, but marking as handled
            instruction_handled = 1;
        }
        uint64_t result = operand1 - operand2;
        update_flags(result);
        if (Rd != 31) {
            CURRENT_STATE.REGS[Rd] = result;
        }
        instruction_handled = 1;
    }
    else if (opcode8 == 0xEB) { // SUBS/CMP Extended Register
        uint32_t Rd = inst & 0x1F;
        uint32_t Rn = (inst >> 5) & 0x1F;
        uint32_t imm3 = (inst >> 10) & 0x7;
        uint32_t option = (inst >> 13) & 0x7;
        uint32_t Rm = (inst >> 16) & 0x1F;
        
        uint64_t operand1 = (Rn == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[Rn];
        uint64_t operand2 = extend_register(CURRENT_STATE.REGS[Rm], option, imm3);
        uint64_t result = operand1 - operand2;
        update_flags(result);            
        if (Rd != 31) {
            CURRENT_STATE.REGS[Rd] = result;
        }
        instruction_handled = 1;
    }
    else if (opcode8 == 0xEA) { // ANDS (shifted register)
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
        instruction_handled = 1;
    }
    else if (opcode8 == 0xCA) { // EOR shifted register
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
        instruction_handled = 1;
    }
    else if (opcode8 == 0xAA) { // ORR Shifted Register
        uint32_t Rd = inst & 0x1F;
        uint32_t Rn = (inst >> 5) & 0x1F;
        uint32_t Rm = (inst >> 16) & 0x1F;
        int64_t op1 = CURRENT_STATE.REGS[Rn];
        int64_t op2 = CURRENT_STATE.REGS[Rm];
        NEXT_STATE.REGS[Rd] = op1 | op2;
        instruction_handled = 1;
    }
    // Check opcode6 patterns if instruction not handled yet
    else if (opcode6 == 0x14) { // B
        int32_t imm26 = inst & 0x3FFFFFF;
        int64_t offset = ((int64_t)(imm26 << 6)) >> 4;
        uint64_t target = CURRENT_STATE.PC + offset;
        CURRENT_STATE.PC = target;
        instruction_handled = 1;
    }
    // Check opcode16 patterns if instruction not handled yet
    else if (opcode16 == 0xD61F) { // BR
        uint32_t Rn = (inst >> 5) & 0x1F;
        uint64_t target = CURRENT_STATE.REGS[Rn];
        CURRENT_STATE.PC = target;
        instruction_handled = 1;
    }
    
    // Uncomment if needed for load/store instructions
    else if (opcode11 == 0x7C0) {  // STUR
        int32_t imm9 = (inst >> 12) & 0x1FF;
        int32_t Rn = (inst >> 5) & 0x1F;
        int32_t Rt = inst & 0x1F;

        imm9 = sign_extend(imm9, 64);
    
        uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
        uint64_t value = CURRENT_STATE.REGS[Rt];
        mem_write_64(addr, value);
        instruction_handled = 1;
    }
    
    else if (opcode11 == 0x1C0) {  // STURB
        int64_t imm9 = (inst >> 12) & 0x1FF; 
        int64_t Rn = (inst >> 5) & 0x1F;     
        int64_t Rt = inst & 0x1F;            
    
        imm9 = sign_extend(imm9, 64);
    
        uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;  
        uint8_t value = (uint8_t) CURRENT_STATE.REGS[Rt];
    
        mem_write_8(addr, value);
        instruction_handled = 1;
    }
        
    else if (opcode11 == 0x3C0) {  // STURH
        int64_t imm9 = (inst >> 12) & 0x1FF; 
        int64_t Rn = (inst >> 5) & 0x1F;     
        int64_t Rt = inst & 0x1F;            
    
        imm9 = sign_extend(imm9, 64);
    
        uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;  
        uint16_t value = (uint16_t) CURRENT_STATE.REGS[Rt];

        mem_write_16(addr, value);
        instruction_handled = 1;
    }

    else if (opcode11 == 0x7C2) {  // LDUR
        int32_t imm9 = (inst >> 12) & 0x1FF;
        int32_t Rn = (inst >> 5) & 0x1F;    
        int32_t Rt = inst & 0x1F;           
    
        imm9 = sign_extend(imm9, 64); 
    
        uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9; 
        uint64_t value = mem_read_64(addr);          
    
        CURRENT_STATE.REGS[Rt] = value;
        instruction_handled = 1;
    }

    else if (opcode11 == 0x1C2) {  // LDURB
        int32_t imm9 = (inst >> 12) & 0x1FF;
        int32_t Rn = (inst >> 5) & 0x1F;    
        int32_t Rt = inst & 0x1F;           
    
        imm9 = sign_extend(imm9, 64); 
    
        uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9; 
        uint8_t value = mem_read_8(addr);             
    
        CURRENT_STATE.REGS[Rt] = (uint64_t)value; 
        instruction_handled = 1;
    }

    else if (opcode11 == 0x3C2) {  // LDURH
        int32_t imm9 = (inst >> 12) & 0x1FF;
        int32_t Rn = (inst >> 5) & 0x1F;
        int32_t Rt = inst & 0x1F;
    
        imm9 = sign_extend(imm9, 64);
    
        uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
        uint16_t value = mem_read_16(addr);
    
        CURRENT_STATE.REGS[Rt] = (uint64_t)value;
        instruction_handled = 1;
    }

    // Print only one "not implemented" message if no handler matched
    if (!instruction_handled) {
        printf("Instruction not implemented: 0x%08x\n", inst);
    }
}