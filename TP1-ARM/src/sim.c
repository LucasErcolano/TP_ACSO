#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "shell.h"
#include "hashmap.h" // Use a lightweight hashmap library

// --------------------------
// Instruction Handler Typedef
// --------------------------
typedef void (*InstructionHandler)(uint32_t);

// --------------------------
// Instruction Metadata Struct
// --------------------------
typedef struct {
    uint32_t pattern;
    int length; 
    InstructionHandler handler;
} InstructionEntry;


// --------------------------
// Global Definitions
// --------------------------
HashMap *opcode_map = NULL;
int branch_taken = 0;

// --------------------------
// Instruction Handlers (Forward Declarations)
// --------------------------
void handle_hlt(uint32_t);
void handle_adds_imm(uint32_t);
void handle_adds_reg(uint32_t);
void handle_subs_imm(uint32_t);
void handle_subs_reg(uint32_t);
void handle_ands(uint32_t);
void handle_eor(uint32_t);
void handle_orr(uint32_t);
void handle_b(uint32_t);
void handle_br(uint32_t);
void handle_b_cond(uint32_t);
void handle_cbz(uint32_t);
void handle_cbnz(uint32_t);
void handle_ldur(uint32_t);
void handle_stur(uint32_t);
void handle_movz(uint32_t);
void handle_lsl(uint32_t);
void handle_lsr(uint32_t);
void handle_mul(uint32_t);
void handle_shifts(uint32_t);
void handle_sturb(uint32_t);
void handle_sturh(uint32_t);
void handle_ldurb(uint32_t);
void handle_ldurh(uint32_t);
void handle_add_reg(uint32_t);
void handle_add_imm(uint32_t);

// --------------------------
// Opcode Map Initialization
// --------------------------
void init_opcode_map() {
    if (opcode_map) return;  // Ensure single initialization
    
    opcode_map = hashmap_create();

    // Format: {pattern, opcode_length, handler}
    InstructionEntry entries[] = {
        // HLT: 11010110000 (8 bits)?
        {0xD4500000, 8, handle_hlt},
        // B.cond: 01010100 (8 bits)
        {0x54000000, 8, handle_b_cond},
        // B: 000101 (6 bits)
        {0x14000000, 6, handle_b},
        // BR: 1101011000111111000000 (22 bits)
        {0xD61F0000, 22, handle_br},
        // ADDS (Immediate): 10110001 (8 bits)
        {0xB1000000, 8, handle_adds_imm},
        // ADDS (Register): 10101011000 (8 bits)?
        {0xAB000000, 8, handle_adds_reg},
        // SUBS (Immediate): 11110001 (8 bits)
        {0xF1000000, 8, handle_subs_imm},
        // SUBS (Register): 11101011000 (8 bits)?
        {0xEB000000, 8, handle_subs_reg},
        // ANDS: 11101010 (8 bits)
        {0xEA000000, 8, handle_ands},
        // EOR: 11001010 (8 bits)
        {0xCA000000, 8, handle_eor},
        // ORR: 10101010 (8 bits)
        {0xAA000000, 8, handle_orr},
        // MOVZ: 110100101 (9 bits)
        {0xD2800000, 9, handle_movz},
        // LSL/LSR: 110100110 (9 bits)
        {0xD3400000, 9, handle_shifts},
        // STUR: 11111000000 (11 bits)
        {0xF8000000, 11, handle_stur},
        // LDUR: 11111000010 (11 bits)
        {0xF8400000, 11, handle_ldur},
        // CBZ/CBNZ: 10110100/10110101 (8 bits)
        {0xB4000000, 8, handle_cbz},
        {0xB5000000, 8, handle_cbnz},
        // MUL: 10011011000 (11 bits)
        {0x9B000000, 11, handle_mul},
        // STURB: (missing opcode_length)
        {0x38000000, 11, handle_sturb},
        // STURH: (missing opcode_length
        {0x78000000, 11, handle_sturh},
        // LDURB: (missing opcode_length
        {0x38400000, 11, handle_ldurb},
        // LDURH: (missing opcode_length
        {0x78400000, 11, handle_ldurh},
        // ADD (Register): (missing opcode_length
        {0x8B000000, 11, handle_add_reg},
        // ADD (Immediate): (missing opcode_length
        {0x91000000, 8, handle_add_imm}
    };

    // Define las máscaras según el opcode_length
    uint32_t mask6 = 0x3F000000;
    uint32_t mask8 = 0xFF000000;
    uint32_t mask9 = 0x1FF00000;
    uint32_t mask11 = 0x7FF0000;
    uint32_t mask16 = 0xFFFF000;
    uint32_t mask22 = 0x3FFFFF0;

    // Register all entries (sorted by descending opcode_length)
    for (int i = 0; i < sizeof(entries)/sizeof(entries[0]); i++) {
        uint32_t mask;
        
        // Seleccionar máscara según opcode_length
        switch (entries[i].length) {
            case 6:
                mask = mask6;
                break;
            case 8:
                mask = mask8;
                break;
            case 9:
                mask = mask9;
                break;
            case 11:
                mask = mask11;
                break;
            case 16:
                mask = mask16;
                break;
            case 22:
                mask = mask22;
                break;
            default:
                // Manejo de error o caso por defecto
                mask = 0xFFFFFFFF;
        }
        
        hashmap_put(opcode_map,
                    mask,
                    entries[i].pattern & mask, // Apply mask to pattern
                    entries[i].handler
        );
    }
}

// --------------------------
// Decoding Logic
// --------------------------
InstructionHandler decode_instruction(uint32_t instruction) {
    if (!opcode_map) init_opcode_map();

    // Iterate through possible opcode lengths (descending order)
    for (int len = 32; len >= 6; len--) {
        uint32_t mask = (0xFFFFFFFF >> (32 - len)) << (32 - len);
        uint32_t opcode = instruction & mask;

        InstructionHandler handler = hashmap_get(opcode_map, mask, opcode);
        if (handler) return handler;
    }
    return NULL;
}

// --------------------------
// Core Processing Function
// --------------------------
void process_instruction() {
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    InstructionHandler handler = decode_instruction(instruction);

    if (handler) {
        branch_taken = 0;
        handler(instruction);
    } else {
        printf("Unsupported instruction: 0x%08X\n", instruction);
        exit(1);
    }
}

// --------------------------
// Helper Functions
// --------------------------
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

// --------------------------
// Instruction Handlers 
// --------------------------
void handle_hlt(uint32_t instruction) {
    // Halt simulation
    RUN_BIT = 0;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_adds_imm(uint32_t instruction) {
    uint64_t imm12 = (instruction >> 10) & 0xFFF;
    uint32_t shift = (instruction >> 22) & 0x1;
    uint32_t Rd = instruction & 0x1F;
    uint32_t Rn = (instruction >> 5) & 0x1F;
    // Keeping the helper function call
    execute_ADDS_immediate(Rd, Rn, imm12, shift);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_adds_reg(uint32_t instruction) {
    uint32_t Rd     = instruction & 0x1F;
    uint32_t Rn     = (instruction >> 5) & 0x1F;
    uint32_t imm3   = (instruction >> 10) & 0x7;
    uint32_t option = (instruction >> 13) & 0x7;
    uint32_t Rm     = (instruction >> 16) & 0x1F;
    // Keeping the helper function call
    execute_ADDS_extended(Rd, Rn, Rm, option, imm3);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_subs_imm(uint32_t instruction) {
    uint64_t imm12 = (instruction >> 10) & 0xFFF;
    uint32_t shift = (instruction >> 22) & 0x3;
    uint32_t Rd = instruction & 0x1F;
    uint32_t Rn = (instruction >> 5) & 0x1F;
    uint64_t operand1 = (Rn == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[Rn];
    uint64_t operand2;
    
    if (shift == 0) {
        operand2 = imm12;
    } else if (shift == 1) {
        operand2 = imm12 << 12;
    } else {
        // Invalid shift, but marking as handled
    }
    uint64_t result = operand1 - operand2;
    update_flags(result);
    if (Rd != 31) {
        CURRENT_STATE.REGS[Rd] = result;
    }
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_subs_reg(uint32_t instruction) {
    uint32_t Rd = instruction & 0x1F;
    uint32_t Rn = (instruction >> 5) & 0x1F;
    uint32_t imm3 = (instruction >> 10) & 0x7;
    uint32_t option = (instruction >> 13) & 0x7;
    uint32_t Rm = (instruction >> 16) & 0x1F;
    
    uint64_t operand1 = (Rn == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[Rn];
    uint64_t operand2 = extend_register(CURRENT_STATE.REGS[Rm], option, imm3);
    uint64_t result = operand1 - operand2;
    update_flags(result);            
    if (Rd != 31) {
        CURRENT_STATE.REGS[Rd] = result;
    }
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ands(uint32_t instruction) {
    uint8_t shift  = (instruction >> 22) & 0x3;
    uint8_t rm     = (instruction >> 16) & 0x1F;
    uint8_t imm6   = (instruction >> 10) & 0x3F;
    uint8_t rn     = (instruction >> 5)  & 0x1F;
    uint8_t rd     = instruction & 0x1F;

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
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_eor(uint32_t instruction) {
    uint32_t Rd = instruction & 0x1F;
    uint32_t Rn = (instruction >> 5) & 0x1F;
    uint32_t imm6 = (instruction >> 10) & 0x3F;
    uint32_t Rm = (instruction >> 16) & 0x1F;
    uint32_t shift = (instruction >> 22) & 0x3;
    
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
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_orr(uint32_t instruction) {
    uint32_t Rd = instruction & 0x1F;
    uint32_t Rn = (instruction >> 5) & 0x1F;
    uint32_t Rm = (instruction >> 16) & 0x1F;
    int64_t op1 = CURRENT_STATE.REGS[Rn];
    int64_t op2 = CURRENT_STATE.REGS[Rm];
    NEXT_STATE.REGS[Rd] = op1 | op2;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_b(uint32_t instruction) {
    int32_t imm26 = instruction & 0x3FFFFFF;
    int64_t offset = ((int64_t)(imm26 << 6)) >> 4;
    uint64_t target = CURRENT_STATE.PC + offset;
    CURRENT_STATE.PC = target;
    branch_taken = 1;
}

void handle_br(uint32_t instruction) {
    uint32_t Rn = (instruction >> 5) & 0x1F;
    uint64_t target = CURRENT_STATE.REGS[Rn];
    CURRENT_STATE.PC = target;
    branch_taken = 1;
}

void handle_stur(uint32_t instruction) {
    int32_t imm9 = (instruction >> 12) & 0x1FF;
    int32_t Rn = (instruction >> 5) & 0x1F;
    int32_t Rt = instruction & 0x1F;

    imm9 = sign_extend(imm9, 64);

    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
    uint64_t value = CURRENT_STATE.REGS[Rt];
    mem_write_64(addr, value);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_sturb(uint32_t instruction) {
    int64_t imm9 = (instruction >> 12) & 0x1FF; 
    int64_t Rn = (instruction >> 5) & 0x1F;     
    int64_t Rt = instruction & 0x1F;            

    imm9 = sign_extend(imm9, 64);

    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;  
    uint8_t value = (uint8_t) CURRENT_STATE.REGS[Rt];

    mem_write_8(addr, value);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_sturh(uint32_t instruction) {
    int64_t imm9 = (instruction >> 12) & 0x1FF; 
    int64_t Rn = (instruction >> 5) & 0x1F;     
    int64_t Rt = instruction & 0x1F;            

    imm9 = sign_extend(imm9, 64);

    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;  
    uint16_t value = (uint16_t) CURRENT_STATE.REGS[Rt];

    mem_write_16(addr, value);
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ldur(uint32_t instruction) {
    int32_t imm9 = (instruction >> 12) & 0x1FF;
    int32_t Rn = (instruction >> 5) & 0x1F;    
    int32_t Rt = instruction & 0x1F;           

    imm9 = sign_extend(imm9, 64); 

    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9; 
    uint64_t value = mem_read_64(addr);          

    CURRENT_STATE.REGS[Rt] = value;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ldurb(uint32_t instruction) {
    int32_t imm9 = (instruction >> 12) & 0x1FF;
    int32_t Rn = (instruction >> 5) & 0x1F;    
    int32_t Rt = instruction & 0x1F;           

    imm9 = sign_extend(imm9, 64); 

    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9; 
    uint8_t value = mem_read_8(addr);             

    CURRENT_STATE.REGS[Rt] = (uint64_t)value; 
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ldurh(uint32_t instruction) {
    int32_t imm9 = (instruction >> 12) & 0x1FF;
    int32_t Rn = (instruction >> 5) & 0x1F;
    int32_t Rt = instruction & 0x1F;

    imm9 = sign_extend(imm9, 64);

    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
    uint16_t value = mem_read_16(addr);

    CURRENT_STATE.REGS[Rt] = (uint64_t)value;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

// Stub implementations for remaining functions
void handle_b_cond(uint32_t instruction) {
    printf("handle_b_cond called but not implemented\n");
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_cbz(uint32_t instruction) {
    printf("handle_cbz called but not implemented\n");
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_cbnz(uint32_t instruction) {
    printf("handle_cbnz called but not implemented\n");
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_movz(uint32_t instruction) {
    printf("handle_movz called but not implemented\n");
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_lsl(uint32_t instruction) {
    printf("handle_lsl called but not implemented\n");
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_lsr(uint32_t instruction) {
    printf("handle_lsr called but not implemented\n");
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_mul(uint32_t instruction) {
    printf("handle_mul called but not implemented\n");
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_shifts(uint32_t instruction) {
    printf("handle_shifts called but not implemented\n");
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_add_reg(uint32_t instruction) {
    printf("handle_add_reg called but not implemented\n");
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_add_imm(uint32_t instruction) {
    printf("handle_add_imm called but not implemented\n");
    if (!branch_taken) NEXT_STATE.PC += 4;
}