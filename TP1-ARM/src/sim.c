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
    uint32_t mask;       // Opcode mask
    uint32_t pattern;    // Expected opcode bits
    int length;          // Opcode length in bits
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

// --------------------------
// Opcode Map Initialization
// --------------------------
void init_opcode_map() {
    if (opcode_map) return;  // Ensure single initialization
    
    opcode_map = hashmap_create();

    // Format: {mask, pattern, opcode_length, handler}
    InstructionEntry entries[] = {
        // HLT: 11010110000 (11 bits)
        {0xFFE00000, 0xD4500000, 11, handle_hlt},

        // B.cond: 01010100 (8 bits)
        {0xFF000000, 0x54000000, 8, handle_b_cond},

        // B: 000101 (6 bits)
        {0xFC000000, 0x14000000, 6, handle_b},

        // BR: 1101011000111111000000 (22 bits)
        {0xFFFFFC00, 0xD61F0000, 22, handle_br},

        // ADDS (Immediate): 10110001 (8 bits)
        {0xFF000000, 0xB1000000, 8, handle_adds_imm},

        // ADDS (Register): 10101011000 (11 bits)
        {0xFF200000, 0xAB000000, 11, handle_adds_reg},

        // SUBS (Immediate): 11110001 (8 bits)
        {0xFF000000, 0xF1000000, 8, handle_subs_imm},

        // SUBS (Register): 11101011000 (11 bits)
        {0xFF200000, 0xEB000000, 11, handle_subs_reg},

        // ANDS: 11101010 (8 bits)
        {0xFF000000, 0xEA000000, 8, handle_ands},

        // EOR: 11001010 (8 bits)
        {0xFF000000, 0xCA000000, 8, handle_eor},

        // ORR: 10101010 (8 bits)
        {0xFF000000, 0xAA000000, 8, handle_orr},

        // MOVZ: 110100101 (9 bits)
        {0xFF800000, 0xD2800000, 9, handle_movz},

        // LSL/LSR: 110100110 (9 bits)
        {0xFFE00000, 0xD3400000, 9, handle_shifts},

        // STUR: 11111000000 (11 bits)
        {0xFFC00000, 0xF8000000, 11, handle_stur},

        // LDUR: 11111000010 (11 bits)
        {0xFFC00000, 0xF8400000, 11, handle_ldur},

        // CBZ/CBNZ: 10110100/10110101 (8 bits)
        {0xFF000000, 0xB4000000, 8, handle_cbz},
        {0xFF000000, 0xB5000000, 8, handle_cbnz},

        // MUL: 10011011000 (11 bits)
        {0xFF200000, 0x9B000000, 11, handle_mul},

        // STURB: (new instruction)
        {0xFFC00000, 0x38000000, 1, handle_sturb},

        // STURH: (new instruction)
        {0xFFC00000, 0x78000000, 1, handle_sturh},

        // LDURB: (new instruction)
        {0xFFC00000, 0x38400000, 1, handle_ldurb},

        // LDURH: (new instruction)
        {0xFFC00000, 0x78400000, 1, handle_ldurh},

        // ADD (Register): (new instruction)
        {0xFF200000, 0x8B000000, 1, handle_add_reg},

        // ADD (Immediate): (new instruction)
        {0xFF000000, 0x91000000, 1, handle_add_imm}
    };
    
    // Register all entries (sorted by descending opcode length)
    for (int i = 0; i < sizeof(entries)/sizeof(entries[0]); i++) {
        hashmap_put(opcode_map, 
            entries[i].mask, 
            entries[i].pattern & entries[i].mask,  // Apply mask to pattern
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
// Instruction Handlers 
// --------------------------
void handle_adds_imm(uint32_t instruction) {
    // Decode fields using bitwise operations
    uint8_t Rd = (instruction >> 0) & 0x1F;
    uint8_t Rn = (instruction >> 5) & 0x1F;
    uint16_t imm12 = (instruction >> 10) & 0xFFF;
    uint8_t shift = (instruction >> 22) & 0x3;

    // Execute and update NEXT_STATE
    int64_t result = CURRENT_STATE.REGS[Rn] + (imm12 << (shift * 12));
    NEXT_STATE.REGS[Rd] = result;

    // Update flags
    NEXT_STATE.FLAG_Z = (result == 0);
    NEXT_STATE.FLAG_N = (result < 0);

    // Update PC if no branch
    if (!branch_taken) NEXT_STATE.PC += 4;
}