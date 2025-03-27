#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h> // For PRIx64
#include "shell.h"
#include "hashmap.h" // Assuming hashmap functions are defined here

// --- Decoder Structs ---
typedef struct {
    uint32_t rd;
    uint32_t rn;
    uint32_t rm;
} Decoded_R;

typedef struct {
    uint32_t rd;
    uint32_t rn;
    uint64_t imm; // Processed immediate
} Decoded_I;

typedef struct {
    uint32_t rt;
    uint32_t rn;
    int64_t  imm9_signed;
} Decoded_D;

typedef struct {
    int64_t offset;
} Decoded_B;

typedef struct {
    uint32_t rn;
} Decoded_BR;

typedef struct {
    uint32_t rt; // Holds Rt for CBZ/CBNZ, Cond for B.cond
    int64_t offset;
} Decoded_CB;

typedef struct {
    uint32_t rd;
    uint32_t hw;
    uint64_t imm16;
} Decoded_IW;

typedef struct {
    uint32_t rd;
    uint32_t rn;
    uint32_t imm6; // Shift amount
} Decoded_ShiftImm;


// --- Global Variables ---
HashMap *opcode_map = NULL;
int branch_taken = 0; // Used to control PC update

// --- Function Prototypes ---
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
void handle_shifts_imm(uint32_t); // Renamed
void handle_sturb(uint32_t);
void handle_sturh(uint32_t);
void handle_ldurb(uint32_t);
void handle_ldurh(uint32_t);
void handle_add_reg(uint32_t);
void handle_add_imm(uint32_t);
void handle_cmp_imm(uint32_t); // For CMP immediate
void handle_cmp_reg(uint32_t); // For CMP register

// --- Utility Functions ---
static inline void update_flags(int64_t result) {
    // Ensure NEXT_STATE is updated, not CURRENT_STATE
    NEXT_STATE.FLAG_Z = (result == 0);
    NEXT_STATE.FLAG_N = (result < 0); // Check sign bit of 64-bit result
}

// Correct Sign Extension
int64_t sign_extend(uint64_t value, int bits) {
    if (bits >= 64) return value; // Avoid over-shifting
    int shift = 64 - bits;
    return (int64_t)(value << shift) >> shift;
}

// Optional: extend_register (Keep if needed for ADD reg, remove if not)
static inline uint64_t extend_register(uint64_t value, int option, int imm3) {
    // This function seems to implement ARMv8's extended register options
    // Keep it if ADD reg requires it, simplify if ADDS/SUBS rules apply
    uint64_t res = value; // Default to no extension/shift
    // Simplified based on common usage, check ARM manual for full details
     switch (option) {
        case 0b000: res = (uint64_t)(uint8_t)value; break; // UXTB
        case 0b001: res = (uint64_t)(uint16_t)value; break; // UXTH
        case 0b010: res = (uint64_t)(uint32_t)value; break; // UXTW
        case 0b011: res = value; break;                     // LSL (UXTX)
        case 0b100: res = (uint64_t)(int8_t)value; break;  // SXTB
        case 0b101: res = (uint64_t)(int16_t)value; break; // SXTH
        case 0b110: res = (uint64_t)(int32_t)value; break; // SXTW
        case 0b111: res = value; break;                     // SXTX
    }
    if (imm3 > 0 && option != 0b011 && option != 0b111) { // Apply shift if specified and not UXTX/SXTX with LSL
         res <<= imm3;
    } else if (imm3 > 0 && (option == 0b011 || option == 0b111)) { // LSL for UXTX/SXTX
         res <<= imm3;
    }
    return res;
}


// --- Memory Access Functions (Using provided mem_read_32/mem_write_32) ---
// These seem correct based on little-endian assumption
uint8_t mem_read_8(uint64_t addr) {
    uint64_t aligned_addr = addr & ~0x3ULL; // Align address to 4 bytes
    uint32_t word = mem_read_32(aligned_addr);
    int byte_offset = addr & 0x3; // Offset within the 32-bit word
    return (word >> (byte_offset * 8)) & 0xFF;
}

uint16_t mem_read_16(uint64_t addr) {
    // Note: ARMv8 generally allows unaligned access, but simulators might simplify
    if (addr & 0x1)
        fprintf(stderr, "Warning: Unaligned halfword read at 0x%" PRIx64 "\n", addr);
    uint64_t aligned_addr = addr & ~0x3ULL;
    uint32_t word = mem_read_32(aligned_addr);
    int half_offset = (addr >> 1) & 0x1; // Offset (0 or 1) for the 16-bit half within the 32-bit word
    return (word >> (half_offset * 16)) & 0xFFFF;
}

uint64_t mem_read_64(uint64_t addr) {
    if (addr & 0x7)
        fprintf(stderr, "Warning: Unaligned 64-bit read at 0x%" PRIx64 "\n", addr);
    // Little-endian: read lower address first for lower 32 bits
    uint32_t low = mem_read_32(addr);
    uint32_t high = mem_read_32(addr + 4);
    return ((uint64_t)high << 32) | low;
}

void mem_write_8(uint64_t addr, uint8_t value) {
    uint64_t aligned_addr = addr & ~0x3ULL;
    uint32_t word = mem_read_32(aligned_addr); // Read the existing word
    int byte_offset = addr & 0x3;
    uint32_t mask = ~(0xFFUL << (byte_offset * 8)); // Mask to clear the byte
    uint32_t shifted_value = (uint32_t)value << (byte_offset * 8);
    word = (word & mask) | shifted_value; // Combine cleared word with new byte
    mem_write_32(aligned_addr, word);      // Write the modified word back
}

void mem_write_16(uint64_t addr, uint16_t value) {
    if (addr & 0x1)
        fprintf(stderr, "Warning: Unaligned halfword write at 0x%" PRIx64 "\n", addr);
    uint64_t aligned_addr = addr & ~0x3ULL;
    uint32_t word = mem_read_32(aligned_addr);
    int half_offset = (addr >> 1) & 0x1; // 0 or 1
    uint32_t mask = ~(0xFFFFUL << (half_offset * 16));
    uint32_t shifted_value = (uint32_t)value << (half_offset * 16);
    word = (word & mask) | shifted_value;
    mem_write_32(aligned_addr, word);
}

void mem_write_64(uint64_t addr, uint64_t value) {
    if (addr & 0x7)
        fprintf(stderr, "Warning: Unaligned 64-bit write at 0x%" PRIx64 "\n", addr);
    // Little-endian: write lower 32 bits to lower address
    mem_write_32(addr, (uint32_t)(value & 0xFFFFFFFFULL));
    mem_write_32(addr + 4, (uint32_t)(value >> 32));
}


// --- Decoder Functions ---
Decoded_R decode_r_type(uint32_t instr) {
    Decoded_R decoded;
    decoded.rd = instr & 0x1F;
    decoded.rn = (instr >> 5) & 0x1F;
    decoded.rm = (instr >> 16) & 0x1F;
    return decoded;
}

Decoded_I decode_i_type(uint32_t instr) {
    Decoded_I decoded;
    decoded.rd = instr & 0x1F;
    decoded.rn = (instr >> 5) & 0x1F;
    uint32_t imm12 = (instr >> 10) & 0xFFF;
    uint32_t shift = (instr >> 22) & 0x3;

    if (shift == 0) { // shift == '00'
        decoded.imm = imm12;
    } else if (shift == 1) { // shift == '01'
        decoded.imm = (uint64_t)imm12 << 12;
    } else {
        // Per spec, only need 00 and 01 for ADD/SUB immediate. Treat others as '00'.
        decoded.imm = imm12;
        // Potentially log warning for unexpected shift value
    }
    return decoded;
}

Decoded_D decode_d_type(uint32_t instr) {
    Decoded_D decoded;
    decoded.rt = instr & 0x1F;
    decoded.rn = (instr >> 5) & 0x1F;
    uint32_t imm9 = (instr >> 12) & 0x1FF;
    decoded.imm9_signed = sign_extend(imm9, 9);
    return decoded;
}

Decoded_B decode_b_type(uint32_t instr) {
    Decoded_B decoded;
    uint32_t imm26 = instr & 0x3FFFFFF;
    decoded.offset = sign_extend(imm26 << 2, 28); // Apply shift *before* sign extending
    return decoded;
}

Decoded_BR decode_br_type(uint32_t instr) {
    Decoded_BR decoded;
    decoded.rn = (instr >> 5) & 0x1F;
    return decoded;
}

Decoded_CB decode_cb_type(uint32_t instr) {
    Decoded_CB decoded;
    decoded.rt = instr & 0x1F; // Holds Rt for CBZ/CBNZ, Cond for B.cond
    uint32_t imm19 = (instr >> 5) & 0x7FFFF;
    decoded.offset = sign_extend(imm19 << 2, 21); // Apply shift *before* sign extending
    return decoded;
}

Decoded_IW decode_iw_type(uint32_t instr) {
    Decoded_IW decoded;
    decoded.rd = instr & 0x1F;
    decoded.imm16 = (instr >> 5) & 0xFFFF;
    decoded.hw = (instr >> 21) & 0x3;
    return decoded;
}

Decoded_ShiftImm decode_shift_immediate(uint32_t instr) {
    Decoded_ShiftImm decoded;
    decoded.rd = instr & 0x1F;
    decoded.rn = (instr >> 5) & 0x1F;
    decoded.imm6 = (instr >> 10) & 0x3F; // Shift amount
    return decoded;
}

// --- Opcode Map Initialization and Decoding ---
void init_opcode_map() {
    if (opcode_map) return;
    opcode_map = hashmap_create();
    // **NOTE:** Ensure patterns and lengths correctly identify unique instructions.
    // CMP instructions might need explicit entries if their top bits differ
    // significantly from SUB/SUBS despite using the same handler logic initially.
    InstructionEntry entries[] = {
        // Opcode bits (shifted) | Length | Handler
        {0xD4400000 >> (32-11), 11, handle_hlt},      // HLT (specific encoding)
        {0x54000000 >> (32-8),  8,  handle_b_cond},   // B.cond
        {0x14000000 >> (32-6),  6,  handle_b},        // B
        {0xD61F0000 >> (32-22), 22, handle_br},       // BR
        {0xB1000000 >> (32-8),  8,  handle_adds_imm}, // ADDS imm
        {0xAB000000 >> (32-8),  8,  handle_adds_reg}, // ADDS reg (simplified)
        {0xF1000000 >> (32-8),  8,  handle_subs_imm}, // SUBS imm / CMP imm
        {0xEB000000 >> (32-8),  8,  handle_subs_reg}, // SUBS reg / CMP reg
        {0xEA000000 >> (32-8),  8,  handle_ands},     // ANDS (simplified)
        {0xCA000000 >> (32-8),  8,  handle_eor},      // EOR (simplified)
        {0xAA000000 >> (32-8),  8,  handle_orr},      // ORR (simplified)
        {0xD2800000 >> (32-9),  9,  handle_movz},     // MOVZ
        {0xD3400000 >> (32-10), 10, handle_shifts_imm},// Shift imm (LSR?) Check Encoding!
        {0xD3800000 >> (32-10), 10, handle_shifts_imm},// Shift imm (LSL?) Check Encoding!
        {0xF8000000 >> (32-11), 11, handle_stur},     // STUR (base)
        {0xF8400000 >> (32-11), 11, handle_ldur},     // LDUR (base)
        {0xB4000000 >> (32-8),  8,  handle_cbz},      // CBZ
        {0xB5000000 >> (32-8),  8,  handle_cbnz},     // CBNZ
        {0x9B000000 >> (32-11), 11, handle_mul},      // MUL (check encoding, might be MADD)
        {0x38000000 >> (32-11), 11, handle_sturb},    // STURB
        {0x78000000 >> (32-11), 11, handle_sturh},    // STURH
        {0x38400000 >> (32-11), 11, handle_ldurb},    // LDURB
        {0x78400000 >> (32-11), 11, handle_ldurh},    // LDURH
        {0x8B000000 >> (32-11), 11, handle_add_reg},  // ADD reg (using extended?)
        {0x91000000 >> (32-8),  8,  handle_add_imm}   // ADD imm
        // Add explicit CMP entries if needed, e.g.,
        // {0xF1000000 >> (32-8),  8,  handle_cmp_imm}, // CMP imm (same pattern as SUBS imm)
        // {0xEB000000 >> (32-8),  8,  handle_cmp_reg}, // CMP reg (same pattern as SUBS reg)
        // The hashmap must handle multiple handlers for the same key if CMP/SUB share patterns
    };
    int n = sizeof(entries) / sizeof(entries[0]);
    for (int i = 0; i < n; i++) {
        // If hashmap allows multiple values per key, this is fine.
        // Otherwise, CMP handlers might overwrite SUB handlers if patterns match.
        hashmap_put(opcode_map, entries[i].length, entries[i].pattern, entries[i].handler);
    }
}

InstructionHandler decode_instruction(uint32_t instruction) {
    if (!opcode_map) init_opcode_map();
    // Order lengths from most specific (longest) to least specific
    static const int lengths[] = {22, 11, 10, 9, 8, 6}; // Adjusted order
    for (int i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        int len = lengths[i];
        uint32_t opcode_key = instruction >> (32 - len);
        InstructionHandler handler = hashmap_get(opcode_map, len, opcode_key);
        if (handler)
            return handler;
    }
    return NULL;
}

// --- Main Execution Logic ---
void process_instruction() {
    // Read instruction pointed by CURRENT_STATE.PC
    uint32_t instr = mem_read_32(CURRENT_STATE.PC);
    InstructionHandler handler = decode_instruction(instr);

    // Reset branch flag before execution
    branch_taken = 0;
    // Set default next PC (can be overwritten by branch instructions)
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;

    if (handler) {
        handler(instr); // Execute the instruction
    } else {
        fprintf(stderr, "Error: Unsupported instruction: 0x%08X at PC=0x%" PRIx64 "\n", instr, CURRENT_STATE.PC);
        // Consider setting RUN_BIT = 0 or calling exit()
        RUN_BIT = 0; // Stop simulation on unknown instruction
        // exit(1); // Or exit immediately
    }

    // Ensure XZR (R31) is always zero in the NEXT state
    NEXT_STATE.REGS[31] = 0;
}


// --- Refactored Instruction Handlers ---

void handle_hlt(uint32_t instr) {
    // HLT stops the simulation loop. PC should not advance.
    RUN_BIT = 0;
    // Override the default PC increment set in process_instruction
    NEXT_STATE.PC = CURRENT_STATE.PC;
}

void handle_adds_imm(uint32_t instr) {
    Decoded_I decoded = decode_i_type(instr);
    uint64_t op1 = CURRENT_STATE.REGS[decoded.rn];
    uint64_t res = op1 + decoded.imm;

    if (decoded.rd != 31) {
        NEXT_STATE.REGS[decoded.rd] = res;
    }
    update_flags(res);
    // PC update handled in process_instruction if branch_taken is 0
}

void handle_adds_reg(uint32_t instr) {
    // Simplified per spec: Ignore extend/amount fields for ADDS
    Decoded_R decoded = decode_r_type(instr);
    uint64_t op1 = CURRENT_STATE.REGS[decoded.rn];
    uint64_t op2 = CURRENT_STATE.REGS[decoded.rm];
    uint64_t res = op1 + op2;

    if (decoded.rd != 31) {
        NEXT_STATE.REGS[decoded.rd] = res;
    }
    update_flags(res);
}

void handle_subs_imm(uint32_t instr) {
    Decoded_I decoded = decode_i_type(instr);
    uint64_t op1 = CURRENT_STATE.REGS[decoded.rn];
    uint64_t res = op1 - decoded.imm;

    update_flags(res); // Update flags even for CMP (Rd=31)
    if (decoded.rd != 31) {
        NEXT_STATE.REGS[decoded.rd] = res;
    }
}

void handle_subs_reg(uint32_t instr) {
    // Simplified per spec: Ignore extend/amount fields for SUBS
    Decoded_R decoded = decode_r_type(instr);
    uint64_t op1 = CURRENT_STATE.REGS[decoded.rn];
    uint64_t op2 = CURRENT_STATE.REGS[decoded.rm];
    uint64_t res = op1 - op2;

    update_flags(res); // Update flags even for CMP (Rd=31)
    if (decoded.rd != 31) {
        NEXT_STATE.REGS[decoded.rd] = res;
    }
}

// CMP handlers just call the SUBS handlers
void handle_cmp_imm(uint32_t instr) { handle_subs_imm(instr); }
void handle_cmp_reg(uint32_t instr) { handle_subs_reg(instr); }


void handle_ands(uint32_t instr) {
    // Simplified: ignore shift amount (shamt/imm6)
    Decoded_R decoded = decode_r_type(instr);
    uint64_t op1 = CURRENT_STATE.REGS[decoded.rn];
    uint64_t op2 = CURRENT_STATE.REGS[decoded.rm];
    uint64_t res = op1 & op2;

    if (decoded.rd != 31) {
        NEXT_STATE.REGS[decoded.rd] = res;
    }
    update_flags(res);
}

void handle_eor(uint32_t instr) {
    // Simplified: ignore shift amount
    Decoded_R decoded = decode_r_type(instr);
    uint64_t op1 = CURRENT_STATE.REGS[decoded.rn];
    uint64_t op2 = CURRENT_STATE.REGS[decoded.rm];
    uint64_t res = op1 ^ op2;

    if (decoded.rd != 31) {
        NEXT_STATE.REGS[decoded.rd] = res;
    }
    // No flag update for plain EOR
}

void handle_orr(uint32_t instr) {
    // Simplified: ignore shift amount
    Decoded_R decoded = decode_r_type(instr);
    uint64_t op1 = CURRENT_STATE.REGS[decoded.rn];
    uint64_t op2 = CURRENT_STATE.REGS[decoded.rm];
    uint64_t res = op1 | op2;

    if (decoded.rd != 31) {
        NEXT_STATE.REGS[decoded.rd] = res;
    }
    // No flag update for plain ORR
}

void handle_b(uint32_t instr) {
    Decoded_B decoded = decode_b_type(instr);
    NEXT_STATE.PC = CURRENT_STATE.PC + decoded.offset; // Update NEXT PC
    branch_taken = 1;
}

void handle_br(uint32_t instr) {
    Decoded_BR decoded = decode_br_type(instr);
    NEXT_STATE.PC = CURRENT_STATE.REGS[decoded.rn]; // Update NEXT PC
    branch_taken = 1;
}

void handle_stur(uint32_t instr) {
    Decoded_D decoded = decode_d_type(instr);
    uint64_t addr = CURRENT_STATE.REGS[decoded.rn] + decoded.imm9_signed;
    uint64_t data = CURRENT_STATE.REGS[decoded.rt]; // Rt is source for store
    mem_write_64(addr, data);
}

void handle_sturb(uint32_t instr) {
    Decoded_D decoded = decode_d_type(instr);
    uint64_t addr = CURRENT_STATE.REGS[decoded.rn] + decoded.imm9_signed;
    uint8_t data = (uint8_t)CURRENT_STATE.REGS[decoded.rt];
    mem_write_8(addr, data);
}

void handle_sturh(uint32_t instr) {
    Decoded_D decoded = decode_d_type(instr);
    uint64_t addr = CURRENT_STATE.REGS[decoded.rn] + decoded.imm9_signed;
    uint16_t data = (uint16_t)CURRENT_STATE.REGS[decoded.rt];
    mem_write_16(addr, data);
}

void handle_ldur(uint32_t instr) {
    Decoded_D decoded = decode_d_type(instr);
    uint64_t addr = CURRENT_STATE.REGS[decoded.rn] + decoded.imm9_signed;
    uint64_t data = mem_read_64(addr);
    if (decoded.rt != 31) { // Rt is destination for load
        NEXT_STATE.REGS[decoded.rt] = data;
    }
}

void handle_ldurb(uint32_t instr) {
    Decoded_D decoded = decode_d_type(instr);
    uint64_t addr = CURRENT_STATE.REGS[decoded.rn] + decoded.imm9_signed;
    uint8_t byte_data = mem_read_8(addr);
    if (decoded.rt != 31) {
        NEXT_STATE.REGS[decoded.rt] = byte_data; // Zero-extended by assignment to uint64_t
    }
}

void handle_ldurh(uint32_t instr) {
    Decoded_D decoded = decode_d_type(instr);
    uint64_t addr = CURRENT_STATE.REGS[decoded.rn] + decoded.imm9_signed;
    uint16_t half_data = mem_read_16(addr);
     if (decoded.rt != 31) {
        NEXT_STATE.REGS[decoded.rt] = half_data; // Zero-extended by assignment
    }
}

void handle_b_cond(uint32_t instr) {
    Decoded_CB decoded = decode_cb_type(instr);
    uint32_t cond = decoded.rt & 0xF; // Condition code in lower bits
    int take_branch = 0;
    int N = CURRENT_STATE.FLAG_N;
    int Z = CURRENT_STATE.FLAG_Z;

    switch (cond) {
        case 0x0: take_branch = (Z == 1); break; // BEQ (Z=1)
        case 0x1: take_branch = (Z == 0); break; // BNE (Z=0)
        case 0xA: take_branch = (N == 0); break; // BGE (N == V -> N == 0)
        case 0xB: take_branch = (N != 0); break; // BLT (N != V -> N != 0)
        case 0xC: take_branch = (Z == 0 && N == 0); break; // BGT (Z=0 && N == V -> Z=0 && N==0)
        case 0xD: take_branch = (Z == 1 || N != 0); break; // BLE (Z=1 || N != V -> Z=1 || N!=0)
        default: break; // Other conditions not required by spec or invalid
    }

    if (take_branch) {
        NEXT_STATE.PC = CURRENT_STATE.PC + decoded.offset;
        branch_taken = 1;
    }
    // If not taken, PC increment is handled by default in process_instruction
}

void handle_movz(uint32_t instr) {
    Decoded_IW decoded = decode_iw_type(instr);
    if (decoded.hw == 0) { // Only implement hw=0 case per spec
        if (decoded.rd != 31) {
            NEXT_STATE.REGS[decoded.rd] = decoded.imm16; // imm16 placed in bits 0-15, rest zeroed
        }
    } else {
         fprintf(stderr,"Warning: MOVZ hw=%u not implemented (instr 0x%08X)\n", decoded.hw, instr);
         // Treat as NOP for Rd != 31, preserve register value
         if (decoded.rd != 31) {
             NEXT_STATE.REGS[decoded.rd] = CURRENT_STATE.REGS[decoded.rd];
         }
    }
}

void handle_add_imm(uint32_t instr) {
    Decoded_I decoded = decode_i_type(instr);
    uint64_t op1 = CURRENT_STATE.REGS[decoded.rn];
    uint64_t res = op1 + decoded.imm;
    if (decoded.rd != 31) {
        NEXT_STATE.REGS[decoded.rd] = res;
    }
    // No flag update
}

void handle_add_reg(uint32_t instr) {
    // Assumes extend_register *is* needed for non-flag setting ADD
    uint32_t rd = instr & 0x1F;
    uint32_t rn = (instr >> 5) & 0x1F;
    uint32_t imm3 = (instr >> 10) & 0x7; // Shift amount for extended reg
    uint32_t opt = (instr >> 13) & 0x7;  // Extension type
    uint32_t rm = (instr >> 16) & 0x1F;

    // **Re-evaluate if simplification applies here too.** If so, remove extend_register.
    uint64_t op1 = CURRENT_STATE.REGS[rn];
    uint64_t op2_extended = extend_register(CURRENT_STATE.REGS[rm], opt, imm3);
    uint64_t res = op1 + op2_extended;

    if (rd != 31) {
        NEXT_STATE.REGS[rd] = res;
    }
    // No flag update
}


void handle_mul(uint32_t instr) {
    Decoded_R decoded = decode_r_type(instr);
    uint64_t op1 = CURRENT_STATE.REGS[decoded.rn];
    uint64_t op2 = CURRENT_STATE.REGS[decoded.rm];
    uint64_t res = op1 * op2;
    if (decoded.rd != 31) {
        NEXT_STATE.REGS[decoded.rd] = res;
    }
    // No flag update
}

void handle_cbz(uint32_t instr) {
    Decoded_CB decoded = decode_cb_type(instr);
    uint32_t rt = decoded.rt; // Register to test

    if (CURRENT_STATE.REGS[rt] == 0) {
        NEXT_STATE.PC = CURRENT_STATE.PC + decoded.offset;
        branch_taken = 1;
    }
    // If not taken, PC increment is handled by default
}

void handle_cbnz(uint32_t instr) {
    Decoded_CB decoded = decode_cb_type(instr);
    uint32_t rt = decoded.rt; // Register to test

    if (CURRENT_STATE.REGS[rt] != 0) {
        NEXT_STATE.PC = CURRENT_STATE.PC + decoded.offset;
        branch_taken = 1;
    }
    // If not taken, PC increment is handled by default
}

void handle_shifts_imm(uint32_t instr) {
    Decoded_ShiftImm decoded = decode_shift_immediate(instr);
    uint64_t operand = CURRENT_STATE.REGS[decoded.rn];
    uint32_t shift_amount = decoded.imm6;
    uint64_t result;

    // Differentiate LSL/LSR based on specific encoding bits.
    // This is a guess based on common patterns - **VERIFY THIS** against ARMv8 manual for SBFM/UBFM.
    // Bit 22 (N) is often involved. Let's assume N=0 for UBFM (LSL) and N=1 for SBFM (LSR).
    uint32_t N_bit = (instr >> 22) & 0x1;

    if (N_bit == 0) { // Guessing LSL (UBFM usually has N=0 or is irrelevant)
        result = operand << shift_amount;
    } else { // Guessing LSR (SBFM usually has N=1)
        result = operand >> shift_amount; // Logical right shift
    }

    if (decoded.rd != 31) {
        NEXT_STATE.REGS[decoded.rd] = result;
    }
    // No flag update
}