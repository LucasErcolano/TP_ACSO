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
    if (opcode_map) return;
    
    opcode_map = hashmap_create();

    // Format: {pattern, opcode_length, handler}
    InstructionEntry entries[] = {
        {0xD4500000, 8, handle_hlt},
        {0x54000000, 8, handle_b_cond},
        {0x14000000, 6, handle_b},
        {0xD61F0000, 22, handle_br},
        {0xB1000000, 8, handle_adds_imm},
        {0xAB000000, 8, handle_adds_reg},
        {0xF1000000, 8, handle_subs_imm},
        {0xEB000000, 8, handle_subs_reg},
        {0xEA000000, 8, handle_ands},
        {0xCA000000, 8, handle_eor},
        {0xAA000000, 8, handle_orr},
        {0xD2800000, 9, handle_movz},
        {0xD3400000, 9, handle_shifts},
        {0xD3800000, 9, handle_shifts},
        {0xF8000000, 11, handle_stur},
        {0xF8400000, 11, handle_ldur},
        {0xB4000000, 8, handle_cbz},
        {0xB5000000, 8, handle_cbnz},
        {0x9B000000, 11, handle_mul},
        {0x38000000, 11, handle_sturb},
        {0x78000000, 11, handle_sturh},
        {0x38400000, 11, handle_ldurb},
        {0x78400000, 11, handle_ldurh},
        {0x8B000000, 11, handle_add_reg},
        {0x91000000, 8, handle_add_imm}
    };

    uint32_t mask6 = 0x3F000000;
    uint32_t mask8 = 0xFF000000;
    uint32_t mask9 = 0x1FF00000;
    uint32_t mask11 = 0x7FF0000;
    uint32_t mask22 = 0x3FFFFF0;

    // Register all entries (sorted by descending opcode_length)
    for (int i = 0; i < sizeof(entries)/sizeof(entries[0]); i++) {
        uint32_t mask;
        
        // Seleccionar máscara según opcode_length (Se podria hacer mas canchero?)
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
            case 22:
                mask = mask22;
                break;
            default:
                printf("Unsupported opcode length: %d\n", entries[i].length);
        }
        
        hashmap_put(opcode_map,
                    mask,
                    entries[i].pattern & mask,
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
    uint32_t imm12 = (instruction >> 10) & 0xFFF;
    uint32_t shift = (instruction >> 22) & 0x3;
    uint32_t d = instruction & 0x1F;
    uint32_t n = (instruction >> 5) & 0x1F;

    uint32_t imm = (shift == 1) ? (imm12 << 12) : imm12;
    uint32_t result = CURRENT_STATE.REGS[n] + imm;
    NEXT_STATE.REGS[d] = result;

    update_flags(result);       
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_adds_reg(uint32_t instruction) {
    uint32_t d     = instruction & 0x1F;
    uint32_t n     = (instruction >> 5) & 0x1F;
    uint32_t imm3   = (instruction >> 10) & 0x7;
    uint32_t option = (instruction >> 13) & 0x7;
    uint32_t m     = (instruction >> 16) & 0x1F;

    uint32_t operand1 = (n == 31) ? CURRENT_STATE.REGS[31] : CURRENT_STATE.REGS[n];
    uint64_t operand2 = extend_register(CURRENT_STATE.REGS[m], option, imm3);
    uint64_t result = operand1 + operand2;
    NEXT_STATE.REGS[d] = result;

    update_flags(result);    
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
        NEXT_STATE.REGS[Rd] = result;
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
        NEXT_STATE.REGS[Rd] = result;
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
    NEXT_STATE.REGS[Rd] = result;
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

    NEXT_STATE.REGS[Rt] = value;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ldurb(uint32_t instruction) {
    int32_t imm9 = (instruction >> 12) & 0x1FF;
    int32_t Rn = (instruction >> 5) & 0x1F;    
    int32_t Rt = instruction & 0x1F;           

    imm9 = sign_extend(imm9, 64); 

    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9; 
    uint8_t value = mem_read_8(addr);             

    NEXT_STATE.REGS[Rt] = (uint64_t)value; 
    if (!branch_taken) NEXT_STATE.PC += 4;
}

void handle_ldurh(uint32_t instruction) {
    int32_t imm9 = (instruction >> 12) & 0x1FF;
    int32_t Rn = (instruction >> 5) & 0x1F;
    int32_t Rt = instruction & 0x1F;

    imm9 = sign_extend(imm9, 64);

    uint64_t addr = CURRENT_STATE.REGS[Rn] + imm9;
    uint16_t value = mem_read_16(addr);

    NEXT_STATE.REGS[Rt] = (uint64_t)value;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

// Stub implementations for remaining functions
void handle_b_cond(uint32_t instruction) {
    // Extraer el campo inmediato de 19 bits (bits [23:5])
    int32_t imm19 = (instruction >> 5) & 0x7FFFF;  // 19 bits
    imm19 = sign_extend(imm19, 19);
    // El offset se obtiene desplazando 2 bits a la izquierda (porque las instrucciones son word-aligned)
    int64_t offset = ((int64_t)imm19) << 2;

    // Extraer el campo de condición (bits [3:0])
    uint32_t cond = instruction & 0xF;
    
    int take_branch = 0;
    switch (cond) {
        case 0x0: // BEQ: Branch if Equal (salta si Z == 1)
            if (CURRENT_STATE.FLAG_Z == 1)
                take_branch = 1;
            break;
        case 0x1: // BNE: Branch if Not Equal (salta si Z == 0)
            if (CURRENT_STATE.FLAG_Z == 0)
                take_branch = 1;
            break;
        case 0xA: // BGE: Branch if Greater or Equal (salta si N == V; como V se asume 0, salta si N == 0)
            if (CURRENT_STATE.FLAG_N == 0)
                take_branch = 1;
            break;
        case 0xB: // BLT: Branch if Less Than (salta si N != V; en nuestro caso, salta si N != 0)
            if (CURRENT_STATE.FLAG_N != 0)
                take_branch = 1;
            break;
        case 0xC: // BGT: Branch if Greater Than (salta si Z == 0 y N == V; en nuestro caso, si Z == 0 y N == 0)
            if ((CURRENT_STATE.FLAG_Z == 0) && (CURRENT_STATE.FLAG_N == 0))
                take_branch = 1;
            break;
        case 0xD: // BLE: Branch if Less or Equal (salta si Z == 1 o N != V; aquí, si Z == 1 o N != 0)
            if ((CURRENT_STATE.FLAG_Z == 1) || (CURRENT_STATE.FLAG_N != 0))
                take_branch = 1;
            break;
        default:
            // Otros códigos de condición no implementados: no se salta.
            break;
    }
    
    if (take_branch) {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        branch_taken = 1;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

// MOVZ
// Formato esperado: movz Rd, imm16, hw
// Solo se implementa el caso en que hw (bits [22:21]) es 0, es decir, sin desplazamiento
// Ejemplo: movz X1, 10   => X1 = 10
void handle_movz(uint32_t instruction) {
    uint32_t rd    = instruction & 0x1F;             // bits [4:0]: destino
    uint32_t hw    = (instruction >> 21) & 0x3;        // bits [22:21]: hw (debe ser 0)
    uint32_t imm16 = (instruction >> 5) & 0xFFFF;      // bits [20:5]: valor inmediato

    if (hw != 0) {
        printf("MOVZ: solo se implementa el caso hw == 0.\n");
    }
    NEXT_STATE.REGS[rd] = imm16;  // Como hw == 0, no se realiza desplazamiento.
    if (!branch_taken) NEXT_STATE.PC += 4;
}

// ADD (Immediate)
// Formato esperado: add Rd, Rn, imm12, shift
// Ejemplo: add X0, X1, 3   => X0 = X1 + 3
// Se implementa tanto shift == 00 (sin desplazamiento) como shift == 01 (imm12 desplazado 12 bits a la izquierda)
void handle_add_imm(uint32_t instruction) {
    uint32_t imm12 = (instruction >> 10) & 0xFFF;    // bits [21:10]: valor inmediato de 12 bits
    uint32_t shift = (instruction >> 22) & 0x3;        // bits [23:22]: indica si se debe desplazar imm12
    uint32_t rd    = instruction & 0x1F;               // bits [4:0]: destino
    uint32_t rn    = (instruction >> 5) & 0x1F;          // bits [9:5]: fuente

    uint64_t imm = (shift == 1) ? (imm12 << 12) : imm12;
    uint64_t result = CURRENT_STATE.REGS[rn] + imm;
    NEXT_STATE.REGS[rd] = result;
    // Nota: ADD no actualiza los flags.
    if (!branch_taken) NEXT_STATE.PC += 4;
}

// ADD (Extended Register)
// Formato esperado: add Rd, Rn, Rm, opc, imm3
// Ejemplo: add X0, X1, X2   => X0 = X1 + X2
// Se utiliza la función extend_register para procesar el valor de Rm según la opción y el imm3.
void handle_add_reg(uint32_t instruction) {
    uint32_t rd     = instruction & 0x1F;             // bits [4:0]: destino
    uint32_t rn     = (instruction >> 5) & 0x1F;        // bits [9:5]: primer operando
    uint32_t imm3   = (instruction >> 10) & 0x7;         // bits [12:10]: valor de desplazamiento para extender
    uint32_t option = (instruction >> 13) & 0x7;         // bits [15:13]: opción de extensión
    uint32_t rm     = (instruction >> 16) & 0x1F;        // bits [20:16]: segundo operando

    uint64_t operand1 = CURRENT_STATE.REGS[rn];
    uint64_t operand2 = extend_register(CURRENT_STATE.REGS[rm], option, imm3);
    uint64_t result = operand1 + operand2;
    NEXT_STATE.REGS[rd] = result;
    // Nota: ADD (register) no actualiza los flags.
    if (!branch_taken) NEXT_STATE.PC += 4;
}

// MUL
// Formato esperado: mul Rd, Rn, Rm
// Ejemplo: mul X0, X1, X2   => X0 = X1 * X2
void handle_mul(uint32_t instruction) {
    uint32_t rd = instruction & 0x1F;                  // bits [4:0]: destino
    uint32_t rn = (instruction >> 5) & 0x1F;             // bits [9:5]: primer operando
    uint32_t rm = (instruction >> 16) & 0x1F;            // bits [20:16]: segundo operando

    uint64_t operand1 = CURRENT_STATE.REGS[rn];
    uint64_t operand2 = CURRENT_STATE.REGS[rm];
    uint64_t result = operand1 * operand2;
    NEXT_STATE.REGS[rd] = result;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

// CBZ (Compare and Branch if Zero)
// Formato esperado: cbz Rt, imm19
// Salta a la dirección (PC + offset) si el registro Rt es cero.
// El offset se obtiene de bits [23:5] (19 bits) y se desplaza 2 bits a la izquierda.
void handle_cbz(uint32_t instruction) {
    uint32_t rt = instruction & 0x1F;                  // bits [4:0]: registro a evaluar
    int32_t imm = (instruction >> 5) & 0x7FFFF;          // bits [23:5]: offset inmediato de 19 bits
    imm = sign_extend(imm, 19);
    int64_t offset = ((int64_t)imm) << 2;                // Desplazar 2 bits para word alignment

    if (CURRENT_STATE.REGS[rt] == 0) {
        CURRENT_STATE.PC = CURRENT_STATE.PC + offset;
        branch_taken = 1;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

// CBNZ (Compare and Branch if Non-Zero)
// Formato esperado: cbnz Rt, imm19
// Salta a la dirección (PC + offset) si el registro Rt NO es cero.
void handle_cbnz(uint32_t instruction) {
    uint32_t rt = instruction & 0x1F;                  // bits [4:0]: registro a evaluar
    int32_t imm = (instruction >> 5) & 0x7FFFF;          // bits [23:5]: offset inmediato de 19 bits
    imm = sign_extend(imm, 19);
    int64_t offset = ((int64_t)imm) << 2;                // Desplazar 2 bits para word alignment

    if (CURRENT_STATE.REGS[rt] != 0) {
        CURRENT_STATE.PC = CURRENT_STATE.PC + offset;
        branch_taken = 1;
    } else {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}

// handle_shifts: Maneja instrucciones de desplazamiento lógico inmediato.
// Se asume que el bit 22 de la instrucción determina el tipo de desplazamiento:
//   - Si el bit 22 es 0, se realiza un LSL (Logical Shift Left).
//   - Si el bit 22 es 1, se realiza un LSR (Logical Shift Right).
//
// Los campos se decodifican de la siguiente forma:
//   - bits [4:0]   : rd (registro destino)
//   - bits [9:5]   : rn (registro fuente)
//   - bits [15:10] : imm6 (valor inmediato para el desplazamiento)
void handle_shifts(uint32_t instruction) {
    // Extraer campos
    uint32_t rd         = instruction & 0x1F;               // bits [4:0]: registro destino
    uint32_t rn         = (instruction >> 5) & 0x1F;          // bits [9:5]: registro fuente
    uint32_t imm6       = (instruction >> 10) & 0x3F;         // bits [15:10]: inmediato de 6 bits
    uint32_t shift_type = (instruction >> 22) & 0x1;           // bit 22: tipo de shift (0: LSL, 1: LSR)

    uint64_t result;
    if (shift_type == 0) {
        // LSL: desplazamiento lógico a la izquierda
        result = CURRENT_STATE.REGS[rn] << imm6;
    } else {
        // LSR: desplazamiento lógico a la derecha
        result = CURRENT_STATE.REGS[rn] >> imm6;
    }

    NEXT_STATE.REGS[rd] = result;
    if (!branch_taken) NEXT_STATE.PC += 4;
}

